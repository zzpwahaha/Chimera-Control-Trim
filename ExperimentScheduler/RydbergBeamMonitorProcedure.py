import numpy as np
from ExperimentProcedure import *
import time


YEAR, MONTH, DAY = today()
exp = ExperimentProcedure()

config_name = "Rydberg_alignment_monitor.Config"
config_path = exp.CONFIGURATION_DIR + config_name
config_file = ConfigurationFile(config_path)

config_file.modify_parameter("REPETITIONS", "Reps:", str(7))
for variable in config_file.config_param.variables:
    config_file.config_param.update_variable(variable.name, scan_type="Constant", scan_dimension=0)    
    
config_file.config_param.update_variable("resonance_scan", scan_type="Variable", new_initial_values=[76], new_final_values=[84])
config_file.config_param.update_scan_dimension(0, range_index=0, variations=33)


def lightshift_scan_420(exp_idx, timeout_control = {'use':False, 'timeout':600}):
    exp_name_prefix = "RYDBERG-ALIGNMENT-420"
    script_name = "rydberg_420_alignment_lightshift.mScript"

    # Update configuration
    config_file.modify_parameter("REPETITIONS", "Reps:", str(6))
    for variable in config_file.config_param.variables:
        config_file.config_param.update_variable(variable.name, scan_type="Constant", scan_dimension=0)    
    config_file.config_param.update_scan_dimension(0, new_ranges=[ScanRange(index=0,left_inclusive=True, right_inclusive=True, variations=26)])
    config_file.config_param.update_variable("resonance_scan", scan_type="Variable", new_initial_values=[-15], new_final_values=[5])
    config_file.save()
    
    # Setup experiment details
    YEAR, MONTH, DAY = today()
    exp_name = f"{exp_name_prefix}-{exp_idx}" 
    exp.open_configuration("\\ExperimentAutomation\\" + config_name)
    exp.open_master_script("\\ExperimentAutomation\\" + script_name)
    exp.run_experiment(exp_name)
    
    # Monitor experiment status
    experiment_monitoring(exp=exp, timeout_control=timeout_control)

    # # Analyze the data
    # data_analysis = da.DataAnalysis(YEAR, MONTH, DAY, exp_name, maximaLocs=analysis_locs.maximaLocs,
    #                         window=window, thresholds=thresholds, binnings=binnings, 
    #                         annotate_title = exp_name, annotate_note=" ")
    # analysis_result = data_analysis.analyze_data()
    # optimal_field = analysis_result[1]
    # print(f"Optimal resoance for {exp_name} is {optimal_field:.3S} ")

    # if (optimal_field.s > 1) or (analysis_result[0].n > 0) or (analysis_result[0].n < -2):
    #     raise ValueError(f"Optimal resoance {optimal_field:.3S} has a variance larger than 1 or {analysis_result[0]:.3S} is outside the normal range, this typically means bad data.")

    # # Update configuration with the optimal field
    # config_file.config_param.update_variable("resonance_scan", constant_value = round(optimal_field.n, 3))
    # config_file.save()
    return 

def lightshift_scan_1013(exp_idx, timeout_control = {'use':False, 'timeout':600}):
    exp_name_prefix = "RYDBERG-ALIGNMENT-1013"
    script_name = "rydberg_1013_alignment.mScript"

    # Update configuration
    config_file.modify_parameter("REPETITIONS", "Reps:", str(6))
    for variable in config_file.config_param.variables:
        config_file.config_param.update_variable(variable.name, scan_type="Constant", scan_dimension=0)    
    config_file.config_param.update_scan_dimension(0, new_ranges=[ScanRange(index=0,left_inclusive=True, right_inclusive=True, variations=26)])
    config_file.config_param.update_variable("resonance_scan", scan_type="Variable", new_initial_values=[-5], new_final_values=[15])
    config_file.save()
    
    # Setup experiment details
    YEAR, MONTH, DAY = today()
    exp_name = f"{exp_name_prefix}-{exp_idx}" 
    exp.open_configuration("\\ExperimentAutomation\\" + config_name)
    exp.open_master_script("\\ExperimentAutomation\\" + script_name)
    exp.run_experiment(exp_name)
    
    # Monitor experiment status
    experiment_monitoring(exp=exp, timeout_control=timeout_control)
    return 

def _calibration():
    exp.setZynqOutput()
    analog_in_calibration(exp=exp, name = "prb_pwr")
    exp.save_all()
    sleep(2)
    analog_in_calibration(exp=exp, name = "op_pwr")
    exp.save_all()
    config_file.reopen()

def exp_run(exp_idx):
    try:
        _calibration()
        lightshift_scan_420(exp_idx=exp_idx, timeout_control = {'use':True, 'timeout':600})
        sleep(5)
        lightshift_scan_1013(exp_idx=exp_idx, timeout_control = {'use':True, 'timeout':600})
        sleep(5)
    except Exception as e:
        print(e)
        exp.hardware_controller.restart_zynq_control()
        return

def rydberg_beam_monitor_procedure():
    for idx in np.arange(10):
        if idx<=7: continue
        print(f"Running experiment sets number {idx}")
        if idx != 8:
            exp.hardware_controller.restart_zynq_control()
        exp_run(idx)


    return

if __name__=='__main__':
    rydberg_beam_monitor_procedure()