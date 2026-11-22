"""Synthetic accounting checks only: not device performance data."""
import importlib.util
from pathlib import Path
spec = importlib.util.spec_from_file_location('analysis', Path(__file__).resolve().parents[1] / 'Results/analyze_results.py')
mod = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)


def row(i, truth, pred, alert, mode='normal'):
    return dict(trial_id=str(i), status='complete', trial_type='acoustic', mode=mode,
                ground_truth=truth, observed_label=pred, alert_observed=alert)


assert mod.summarize([])['data_status'] == 'no scored data'
fixtures = [row(1,'knock','knock','yes'), row(2,'knock','none','no'),
            row(3,'doorbell','knock','yes'), row(4,'other','doorbell','yes'),
            row(5,'none','none','no'), row(6,'doorbell','other','no','knock')]
fixtures.append(dict(trial_id='7', status='complete', trial_type='background', mode='normal',
                     ground_truth='other', exposure_s='600', false_alert_activations='2'))
r = mod.summarize(fixtures)['by_mode']
assert r['normal']['exact_label_agreement']['fraction'] == 2/5
assert r['normal']['per_target']['knock']['precision']['fraction'] == 1/2
assert r['normal']['per_target']['knock']['recall']['fraction'] == 1/2
assert r['normal']['target_trials_without_target_label'] == 1
assert r['normal']['wrong_target_classifications'] == 1
assert r['normal']['background_false_alerts_per_hour'] == 12
assert r['knock']['eligible_target_trials'] == 0
assert r['knock']['unexpected_physical_alerts']['numerator'] == 0
try:
    mod.summarize([fixtures[0], fixtures[0]])
    raise AssertionError('Duplicate ID accepted')
except ValueError:
    pass
bad = dict(fixtures[-1], exposure_s='0')
try:
    mod.summarize([bad])
    raise AssertionError('Zero exposure accepted')
except ValueError:
    pass
assert len(mod.summarize([dict(fixtures[0], status='incomplete')])['excluded_rows']) == 1
print('Synthetic accounting checks passed; no device metrics generated.')
