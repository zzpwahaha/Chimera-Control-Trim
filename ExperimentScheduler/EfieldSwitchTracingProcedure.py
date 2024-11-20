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

    if (optimal_field.s > 1) or (analysis_result[0].n > 0) or (analysis_result[0].n < -2):
        raise ValueError(f"Optimal resoance {optimal_field:.3S} has a variance larger than 1 or {analysis_result[0]:.3S} is outside the normal range, this typically means bad data.")

    # # Update configuration with the optimal field
    # config_file.config_param.update_variable("resonance_scan", constant_value = round(optimal_field.n, 3))
    # config_file.save()
    return 

def efield(scan_name_prefix, e_field_name, e_field_value1, e_field_value2):
    START_NUM = 0
    SCAN_NUM1 = 30
    SCAN_NUM2 = 5

    config_file.config_param.update_variable(e_field_name, constant_value = e_field_value1)
    config_file.save()
    exp.open_configuration("\\ExperimentAutomation\\" + config_name)

    for idx in np.arange(SCAN_NUM1):
        idx = idx + START_NUM
        if idx != 0:
            exp.hardware_controller.restart_zynq_control()
        try:
            exp.setZynqOutput()
            analog_in_calibration(exp=exp, name = "prb_pwr")
            exp.save_all()
            config_file.reopen()
            resonace_scan(exp_idx=idx, exp_name_prefix=scan_name_prefix, timeout_control = {'use':True, 'timeout':600})
        except Exception as e:
            print(e)
            # exp.hardware_controller.restart_zynq_control()

    config_file.config_param.update_variable(e_field_name, constant_value = e_field_value2)
    config_file.save()
    exp.open_configuration("\\ExperimentAutomation\\" + config_name)
    for idx in np.arange(SCAN_NUM2):
        idx = idx+SCAN_NUM1+START_NUM
        if idx != 0:
            exp.hardware_controller.restart_zynq_control()
        try:
            exp.setZynqOutput()
            analog_in_calibration(exp=exp, name = "prb_pwr")
            exp.save_all()
            config_file.reopen()
            resonace_scan(exp_idx=idx, exp_name_prefix=scan_name_prefix, timeout_control = {'use':True, 'timeout':600})
        except Exception as e:
            print(e)
            # exp.hardware_controller.restart_zynq_control()


def efield_tracing_procedure():
    # efield(scan_name_prefix="EFIELD-RESONANCE-SCAN-X-PLUS", e_field_name='bias_e_x', e_field_value1= 0.53+1.0, e_field_value2= 0.53)
    # sleep(10)
    # efield(scan_name_prefix="EFIELD-RESONANCE-SCAN-X-MINUS", e_field_name='bias_e_x', e_field_value1= 0.53-1.0, e_field_value2= 0.53)
    # sleep(10)
    efield(scan_name_prefix="EFIELD-RESONANCE-SCAN-Y-PLUS", e_field_name='bias_e_y', e_field_value1=-0.42+0.8, e_field_value2=-0.42)
    sleep(10)
    efield(scan_name_prefix="EFIELD-RESONANCE-SCAN-Y-MINUS", e_field_name='bias_e_y', e_field_value1=-0.42-0.8, e_field_value2=-0.42)
    sleep(10)
    efield(scan_name_prefix="EFIELD-RESONANCE-SCAN-Z-PLUS", e_field_name='bias_e_z', e_field_value1= 0.06+1.0, e_field_value2= 0.06)
    sleep(10)
    efield(scan_name_prefix="EFIELD-RESONANCE-SCAN-Z-MINUS", e_field_name='bias_e_z', e_field_value1= 0.06-1.0, e_field_value2=0.06)

if __name__=='__main__':
    efield_tracing_procedure()
