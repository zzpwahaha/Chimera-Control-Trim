import numpy as np
from ExperimentProcedure import *
import time


YEAR, MONTH, DAY = today()
exp = ExperimentProcedure()

config_name = "Rydberg_Rabi_Ramsey.Config"
config_path = exp.CONFIGURATION_DIR + config_name
config_file = ConfigurationFile(config_path)

config_file.modify_parameter("REPETITIONS", "Reps:", str(7))
for variable in config_file.config_param.variables:
    config_file.config_param.update_variable(variable.name, scan_type="Constant", scan_dimension=0)    
    
config_file.config_param.update_variable("resonance_scan", scan_type="Variable", new_initial_values=[76], new_final_values=[84])
config_file.config_param.update_scan_dimension(0, range_index=0, variations=33)

# analysis grid
window = [0, 0, 200, 30]
thresholds = 65
binnings = np.linspace(0, 240, 241)
analysis_locs = da.DataAnalysis(year='2024', month='August', day='26', data_name='data_1', 
                                window=window, thresholds=70, binnings=binnings)

def resonace_scan(exp_idx, exp_name_prefix, timeout_control = {'use':False, 'timeout':600}):
    script_name = "Calibration_rydberg_420_1013_excitation.mScript"

    # Update configuration
    config_file.modify_parameter("REPETITIONS", "Reps:", str(4))
    for variable in config_file.config_param.variables:
        config_file.config_param.update_variable(variable.name, scan_type="Constant", scan_dimension=0)    
    config_file.config_param.update_scan_dimension(0, new_ranges=[ScanRange(index=0,left_inclusive=True, right_inclusive=True, variations=22)])
    config_file.config_param.update_variable("resonance_scan", scan_type="Variable", new_initial_values=[73], new_final_values=[87])
    config_file.save()
    
    # Setup experiment details
    YEAR, MONTH, DAY = today()
    exp_name = f"{exp_name_prefix}-{exp_idx}" #EFIELD-RESONANCE-SCAN-Z-MINUS
    exp.open_configuration("\\ExperimentAutomation\\" + config_name)
    exp.open_master_script("\\ExperimentAutomation\\" + script_name)
    exp.run_experiment(exp_name)
    
    # Monitor experiment status
    experiment_monitoring(exp=exp, timeout_control=timeout_control)

    # Analyze the data
    data_analysis = da.DataAnalysis(YEAR, MONTH, DAY, exp_name, maximaLocs=analysis_locs.maximaLocs,
                            window=window, thresholds=thresholds, binnings=binnings, 
                            annotate_title = exp_name, annotate_note=" ")
    analysis_result = data_analysis.analyze_data()
    optimal_field = analysis_result[1]
    print(f"Optimal resoance for {exp_name} is {optimal_field:.3S} ")

    fit_fail = (optimal_field.s > 1) or (analysis_result[0].n > 0) or (analysis_result[0].n < -2)
    if fit_fail:
        raise ValueError(f"Optimal resoance {optimal_field:.3S} has a variance larger than 1 or {analysis_result[0]:.3S} is outside the normal range, this typically means bad data.")

    # # Update configuration with the optimal field
    # config_file.config_param.update_variable("resonance_scan", constant_value = round(optimal_field.n, 3))
    # config_file.save()
    return fit_fail

def _calibration():
    exp.setZynqOutput()
    analog_in_calibration(exp=exp, name = "prb_pwr")
    exp.save_all()
    config_file.reopen()

def _raw_move_EOM_resonance(exp:ExperimentProcedure, start_freq, end_freq, step = 0.1):
    if start_freq == end_freq:
        return
    freqs = np.arange(start_freq, end_freq, step)
    for f in freqs:
        exp.setStaticDDS(ddsfreq=f, channel=0)
        sleep(0.25)
    exp.setStaticDDS(ddsfreq=end_freq, channel=0)
    sleep(0.5)

def _move_EOM_resonance(start_freq, end_freq, step = 0.1):
    _raw_move_EOM_resonance(exp=exp, start_freq=start_freq, end_freq=end_freq, step=step)
    exp.save_all()
    config_file.reopen()

def high_to_low_direction(exp_idx):
    EOM_center_freqs = np.linspace(225, 255, 4)
    previous_f = 225
    for idx, eom_f in enumerate(EOM_center_freqs):
        _move_EOM_resonance(previous_f, eom_f, step=0.1)
        previous_f = eom_f
        _calibration()
        sleep(1)
        try:
            resonace_scan(exp_idx=exp_idx, exp_name_prefix=f"RESONANCE-SCAN-EOM{eom_f}-DESCEND")
        except:
            sleep(10)
            exp.hardware_controller.restart_zynq_control()
        sleep(10)
        exp.hardware_controller.restart_zynq_control()

def low_to_high_direction(exp_idx):
    EOM_center_freqs = np.linspace(255, 205, 6)
    previous_f = 255
    for idx, eom_f in enumerate(EOM_center_freqs):
        _move_EOM_resonance(previous_f, eom_f, step=-0.1)
        previous_f = eom_f
        _calibration()
        sleep(1)
        try:
            resonace_scan(exp_idx=exp_idx, exp_name_prefix=f"RESONANCE-SCAN-EOM{eom_f}-ASCEND")
        except:
            sleep(10)
            exp.hardware_controller.restart_zynq_control()
        sleep(10)
        exp.hardware_controller.restart_zynq_control()


def efield_tracing_procedure():
    for idx in np.arange(1):
        # if idx<=0: continue
        # print(f"Running experiment sets number {idx}")
        # if idx != 0:
        #     exp.hardware_controller.restart_zynq_control()
        high_to_low_direction(1)
        # low_to_high_direction(idx)


if __name__=='__main__':
    efield_tracing_procedure()
    # _raw_move_EOM_resonance(exp, start_freq=581,end_freq=586,step=0.1)
    # _raw_move_EOM_resonance(exp, start_freq=205,end_freq=225,step=0.25)
    # _raw_move_EOM_resonance(exp, start_freq=324,end_freq=224,step=-0.5)