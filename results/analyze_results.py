"""Summarize manually observed SilentSignal trials; Python standard library only."""
import argparse
import csv
import json
import math
from collections import Counter

CLASSES = ('none', 'other', 'knock', 'doorbell')
TARGETS = {'knock', 'doorbell'}
MODES = {'normal', 'knock', 'doorbell'}


def ratio(n, d):
    return {'numerator': n, 'denominator': d, 'fraction': n / d if d else None}


def summarize(rows):
    groups = {}
    seen = set()
    excluded = []
    for line, r in enumerate(rows, 2):
        if not any(str(v or '').strip() for v in r.values()):
            continue
        ident = r.get('trial_id', '').strip()
        if not ident or ident in seen:
            raise ValueError(f'Line {line}: trial_id must be nonempty and unique')
        seen.add(ident)
        if r.get('status', '').strip() != 'complete':
            excluded.append({'trial_id': ident, 'reason': 'status is not complete'})
            continue
        kind = r.get('trial_type', '').strip()
        if kind == 'control':
            excluded.append({'trial_id': ident, 'reason': 'control test; review output fields manually'})
            continue
        if kind not in {'acoustic', 'background'}:
            raise ValueError(f'{ident}: trial_type must be acoustic, background or control')
        mode = r.get('mode', '').strip()
        if mode not in MODES:
            raise ValueError(f'{ident}: invalid mode')
        g = groups.setdefault(mode, {'events': [], 'background_seconds': 0.0,
                                     'background_activations': 0, 'background_sessions': 0})
        if kind == 'background':
            seconds = float(r.get('exposure_s', ''))
            count = int(r.get('false_alert_activations', ''))
            if not math.isfinite(seconds) or seconds <= 0 or count < 0:
                raise ValueError(f'{ident}: require positive finite exposure_s and nonnegative integer count')
            if r.get('ground_truth') not in {'none', 'other'}:
                raise ValueError(f'{ident}: background sessions cannot include target events')
            g['background_seconds'] += seconds
            g['background_activations'] += count
            g['background_sessions'] += 1
            continue
        truth = r.get('ground_truth', '').strip()
        predicted = r.get('observed_label', '').strip()
        alert = r.get('alert_observed', '').strip()
        if truth not in CLASSES or predicted not in CLASSES or alert not in {'yes', 'no'}:
            raise ValueError(f'{ident}: invalid class or missing yes/no alert_observed')
        if mode != 'normal' and ((mode == 'knock' and predicted == 'doorbell') or
                                 (mode == 'doorbell' and predicted == 'knock')):
            raise ValueError(f'{ident}: excluded target label contradicts mode-filtered OLED; check stale state')
        g['events'].append((truth, predicted, alert == 'yes'))

    output = {'data_status': 'no scored data' if not groups else 'manually recorded observations',
              'excluded_rows': excluded, 'by_mode': {}}
    for mode, g in sorted(groups.items()):
        events = g['events']
        eligible = lambda truth: truth in TARGETS and (mode == 'normal' or truth == mode)
        valid_targets = [e for e in events if eligible(e[0])]
        noneligible = [e for e in events if not eligible(e[0])]
        stats = {'acoustic_trials': len(events),
                 'eligible_target_trials': len(valid_targets),
                 'eligible_targets_without_physical_alert': sum(not e[2] for e in valid_targets),
                 'unexpected_physical_alerts': ratio(sum(e[2] for e in noneligible), len(noneligible)),
                 'background_sessions': g['background_sessions'],
                 'background_exposure_s': g['background_seconds'],
                 'background_false_alert_activations': g['background_activations'],
                 'background_false_alerts_per_hour': (g['background_activations'] * 3600 /
                      g['background_seconds']) if g['background_seconds'] else None}
        if mode == 'normal':
            matrix = {t: {p: 0 for p in CLASSES} for t in CLASSES}
            for truth, pred, _ in events:
                matrix[truth][pred] += 1
            stats['confusion_matrix_rows_truth_columns_new_display_label'] = matrix
            stats['exact_label_agreement'] = ratio(sum(t == p for t, p, _ in events), len(events))
            stats['target_trials_without_target_label'] = sum(p not in TARGETS for _, p, _ in valid_targets)
            stats['wrong_target_classifications'] = sum(p in TARGETS and p != t for t, p, _ in valid_targets)
            stats['per_target'] = {}
            for c in sorted(TARGETS):
                tp = matrix[c][c]
                fp = sum(matrix[t][c] for t in CLASSES if t != c)
                fn = sum(matrix[c][p] for p in CLASSES if p != c)
                stats['per_target'][c] = {'TP': tp, 'FP': fp, 'FN': fn,
                                         'precision': ratio(tp, tp + fp), 'recall': ratio(tp, tp + fn)}
        else:
            stats['classification_note'] = 'OLED labels are mode filtered; raw classifier accuracy is not inferred.'
        output['by_mode'][mode] = stats
    return output


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('csv_file')
    args = parser.parse_args()
    try:
        with open(args.csv_file, newline='', encoding='utf-8-sig') as f:
            reader = csv.DictReader(f)
            required = {'trial_id', 'status', 'trial_type', 'mode', 'ground_truth',
                        'observed_label', 'alert_observed', 'exposure_s', 'false_alert_activations'}
            if not required.issubset(reader.fieldnames or []):
                raise ValueError('Missing required CSV columns: ' + ', '.join(sorted(required - set(reader.fieldnames or []))))
            report = summarize(reader)
        print(json.dumps(report, indent=2, allow_nan=False))
    except (OSError, ValueError, TypeError) as exc:
        parser.error(str(exc))


if __name__ == '__main__':
    main()
