import numpy as np
from ExperimentProcedure import *
from RydbergBeamMoveProcedure import move_beam_to_target
import time


YEAR, MONTH, DAY = today()
exp = ExperimentProcedure()

# analysis grid
window = [0, 0, 200, 30]
thresholds = 100
binnings = np.linspace(0, 240, 241)
analysis_locs = da.DataAnalysis(year='2024', month='August', day='26', data_name='data_1', 
                                window=window, thresholds=70, binnings=binnings)



def rydberg_420_lightshift(exp_postfix: str, timeout_control = {'use':False, 'timeout':600}):
    exp_name_prefix = "RYDBERG-LIGHTSHIFT-420"
    script_name = "rydberg_420_alignment_lightshift.mScript"

    config_name = "420alignment.Config"
    config_path = exp.CONFIGURATION_DIR + config_name
    config_file = ConfigurationFile(config_path)

    # Update configuration
    config_file.modify_parameter("REPETITIONS", "Reps:", str(6))
    config_file.modify_parameter("MAIN_OPTIONS", "Randomize Variations?", str(1))
    config_file.modify_parameter("MAIN_OPTIONS", "Repetition First Over Variation?", str(0))
    config_file.modify_parameter("PICOSCREW", "Control?", str(0))
    for variable in config_file.config_param.variables:
        config_file.config_param.update_variable(variable.name, scan_type="Constant", scan_dimension=0)    
    config_file.config_param.update_variable("resonance_scan", scan_type="Variable", new_initial_values=[-15], new_final_values=[10])
    config_file.config_param.update_scan_dimension(0, range_index=0, variations=26)

    config_file.save()

    # Setup experiment details
    YEAR, MONTH, DAY = today()
    exp_name = f"{exp_name_prefix}-{exp_postfix}" 
    exp.open_configuration("\\ExperimentAutomation\\" + config_name)
    exp.open_master_script("\\ExperimentAutomation\\" + script_name)
    
    start_and_stop_camera(exp)

    _calibration(exp=exp, config_file=config_file)
    
    exp.run_experiment(exp_name)
    
    # Monitor experiment status
    experiment_monitoring(exp=exp, timeout_control=timeout_control)

    # Analyze the data
    data_analysis = da.DataAnalysis(YEAR, MONTH, DAY, exp_name, maximaLocs=analysis_locs.maximaLocs,
                            window=window, thresholds=thresholds, binnings=binnings, 
                            annotate_title = exp_name, annotate_note=" ")
    analysis_result = data_analysis.analyze_data()
    optimal_field = analysis_result[1]
    vertical_pos, horizontal_pos = data_analysis.analyze_mako_data(mako_idx=3, function = da.gaussian)
    print(f"420 lightshift for {exp_name} is {optimal_field:.3S}  with"
          f" position ({vertical_pos.mean():.3S}, {horizontal_pos.mean():.3S})")

def rydberg_1013_lightshift(exp_postfix: str, timeout_control = {'use':False, 'timeout':600}):
    exp_name_prefix = "RYDBERG-LIGHTSHIFT-1013"
    script_name = "rydberg_1013_alignment.mScript"

    config_name = "1013alignment.Config"
    config_path = exp.CONFIGURATION_DIR + config_name
    config_file = ConfigurationFile(config_path)

    # Update configuration
    config_file.modify_parameter("REPETITIONS", "Reps:", str(6))
    config_file.modify_parameter("MAIN_OPTIONS", "Randomize Variations?", str(1))
    config_file.modify_parameter("MAIN_OPTIONS", "Repetition First Over Variation?", str(0))
    config_file.modify_parameter("PICOSCREW", "Control?", str(0))
    for variable in config_file.config_param.variables:
        config_file.config_param.update_variable(variable.name, scan_type="Constant", scan_dimension=0)    
    config_file.config_param.update_variable("resonance_scan", scan_type="Variable", new_initial_values=[-10], new_final_values=[15])
    config_file.config_param.update_scan_dimension(0, range_index=0, variations=26)

    config_file.save()

    # Setup experiment details
    YEAR, MONTH, DAY = today()
    exp_name = f"{exp_name_prefix}-{exp_postfix}" 
    exp.open_configuration("\\ExperimentAutomation\\" + config_name)
    exp.open_master_script("\\ExperimentAutomation\\" + script_name)
    
    start_and_stop_camera(exp)

    _calibration(exp=exp, config_file=config_file)
    
    exp.run_experiment(exp_name)
    
    # Monitor experiment status
    experiment_monitoring(exp=exp, timeout_control=timeout_control)

    # Analyze the data
    data_analysis = da.DataAnalysis(YEAR, MONTH, DAY, exp_name, maximaLocs=analysis_locs.maximaLocs,
                            window=window, thresholds=thresholds, binnings=binnings, 
                            annotate_title = exp_name, annotate_note=" ")
    analysis_result = data_analysis.analyze_data()
    optimal_field = analysis_result[1]
    vertical_pos, horizontal_pos = data_analysis.analyze_mako_data(mako_idx=4, function = da.gaussian)
    print(f"1013 lightshift for {exp_name} is {optimal_field:.3S}  with"
          f" position ({vertical_pos.mean():.3S}, {horizontal_pos.mean():.3S})")

def rydberg_1013_lightshift_DLS(exp_postfix: str, timeout_control = {'use':False, 'timeout':600}):
    exp_name_prefix = "RYDBERG-LIGHTSHIFT-DLS-1013"
    script_name = "rydberg_1013_MWlightshift.mScript"

    config_name = "1013alignment_with_microwave.Config"
    config_path = exp.CONFIGURATION_DIR + config_name
    config_file = ConfigurationFile(config_path)

    # Update configuration
    config_file.modify_parameter("REPETITIONS", "Reps:", str(4))
    config_file.modify_parameter("MAIN_OPTIONS", "Randomize Variations?", str(1))
    config_file.modify_parameter("MAIN_OPTIONS", "Repetition First Over Variation?", str(0))
    config_file.modify_parameter("PICOSCREW", "Control?", str(0))
    for variable in config_file.config_param.variables:
        config_file.config_param.update_variable(variable.name, scan_type="Constant", scan_dimension=0)    
    config_file.config_param.update_variable("time_scan", constant_value = 0.11)
    config_file.config_param.update_variable("mw_resonance", scan_type="Variable", new_initial_values=[7.15], new_final_values=[7.23])
    config_file.config_param.update_scan_dimension(0, range_index=0, variations=25)

    config_file.save()

    # Setup experiment details
    YEAR, MONTH, DAY = today()
    exp_name = f"{exp_name_prefix}-{exp_postfix}" 
    exp.open_configuration("\\ExperimentAutomation\\" + config_name)
    exp.open_master_script("\\ExperimentAutomation\\" + script_name)
    
    start_and_stop_camera(exp)

    _calibration(exp=exp, config_file=config_file)
    
    exp.run_experiment(exp_name)
    
    # Monitor experiment status
    experiment_monitoring(exp=exp, timeout_control=timeout_control)

    # Analyze the data
    data_analysis = da.DataAnalysis(YEAR, MONTH, DAY, exp_name, maximaLocs=analysis_locs.maximaLocs,
                            window=window, thresholds=thresholds, binnings=binnings, 
                            annotate_title = exp_name, annotate_note=" ")
    analysis_result = data_analysis.analyze_data()
    optimal_field = analysis_result[1]
    vertical_pos, horizontal_pos = data_analysis.analyze_mako_data(mako_idx=4, function = da.gaussian)
    print(f"1013 lightshift for {exp_name} is {optimal_field:.3S}  with"
          f" position ({vertical_pos.mean():.3S}, {horizontal_pos.mean():.3S})")

def rydberg_420_alignment(exp_postfix: str, pico_idx: int, timeout_control = {'use':False, 'timeout':1200}):
    NUM_POSITION_VAR = 9
    if pico_idx not in [1,2]:
        raise ValueError("pico_idx out of the range. Ranges are " + str([1,2,3,4]))
    VERTICAL = 1; HORIZONTAL = 2;
    align_axis = -1
    if pico_idx==1:
        align_axis = VERTICAL
    else:
        align_axis = HORIZONTAL
    exp_name_prefix = "RYDBERG-ALIGNMENT-420"
    script_name = "rydberg_420_alignment_lightshift.mScript"

    config_name = "420alignment.Config"
    config_path = exp.CONFIGURATION_DIR + config_name
    config_file = ConfigurationFile(config_path)

    # # Update configuration
    config_file.modify_parameter("REPETITIONS", "Reps:", str(5))
    config_file.modify_parameter("MAIN_OPTIONS", "Randomize Variations?", str(0))
    config_file.modify_parameter("MAIN_OPTIONS", "Repetition First Over Variation?", str(1))
    config_file.modify_parameter("PICOSCREW", "Control?", str(1))
    for idx in range(4):
        config_file.modify_parameter("PICOSCREW", f" Screw-{idx+1} Value:", "0")
    config_file.modify_parameter("PICOSCREW", f" Screw-{pico_idx} Value:", "integer_scan")
    
    for variable in config_file.config_param.variables:
        config_file.config_param.update_variable(variable.name, scan_type="Constant", scan_dimension=0)    
    config_file.config_param.update_variable("integer_scan", scan_type="Variable", scan_dimension=0, new_initial_values=[-200], new_final_values=[200])
    config_file.config_param.update_variable("resonance_scan", scan_type="Variable", scan_dimension=1, new_initial_values=[-10], new_final_values=[5])
    config_file.config_param.update_scan_dimension(0, range_index=0, variations=NUM_POSITION_VAR)
    config_file.config_param.update_scan_dimension(1, range_index=0, variations=11)
    config_file.save()

    exp.setPicoScrewHomes()

    # # Setup experiment details
    YEAR, MONTH, DAY = today()
    if align_axis==VERTICAL: axis_str = "VERTICAL"
    else: axis_str = "HORIZONTAL"
    exp_name = f"{exp_name_prefix}-{axis_str}-{exp_postfix}" 
    exp.open_configuration("\\ExperimentAutomation\\" + config_name)
    exp.open_master_script("\\ExperimentAutomation\\" + script_name)

    _calibration(exp=exp, config_file=config_file)

    exp.run_experiment(exp_name)
    
    # # Monitor experiment status
    experiment_monitoring(exp=exp, timeout_control=timeout_control)

    # Analyze the data
    data_analysis = da.DataAnalysis(YEAR, MONTH, DAY, exp_name, maximaLocs=analysis_locs.maximaLocs,
                            window=window, thresholds=thresholds, binnings=binnings, 
                            annotate_title = exp_name, annotate_note=" ")
    
    vertical_pos, horizontal_pos = data_analysis.analyze_mako_data(mako_idx=3, function = da.gaussian)
    if align_axis==VERTICAL: xkey = da.ah.nominal(vertical_pos).reshape(NUM_POSITION_VAR,11).mean(axis=1)
    else: xkey = da.ah.nominal(horizontal_pos).reshape(NUM_POSITION_VAR,11).mean(axis=1)
    analysis_result = data_analysis.analyze_data_2D(xkey0=xkey, function_d0=da.gaussian, function_d1=da.gaussian)
    optimal_field = analysis_result[1]
    
    if align_axis == VERTICAL:
        print_str = f"position ({optimal_field:.3S}, {horizontal_pos.mean():.3S})"
        target_position = (optimal_field.n, horizontal_pos.mean().n)
    else:
        print_str = f"position (vertical,horizontal) ({vertical_pos.mean():.3S}, {optimal_field:.3S})"
        target_position = (vertical_pos.mean().n, optimal_field.n)

    print(f"420 lightshift for {exp_name} is {optimal_field:.3S}  with " + print_str)
    screws_position = exp.getPicoScrewPositions()
    exp.setPicoScrewPosition(1,screws_position[0], update=False)
    exp.setPicoScrewPosition(2,screws_position[1], update=False)
    move_beam_to_target(exp, mako_idx=3, pico_idx=(1,2), target_position=target_position, tolerance=0.3)
    sleep(1)
    exp.setPicoScrewHomes()
    exp.setPicoScrewPosition(1,position=0, update=False)
    exp.setPicoScrewPosition(2,position=0, update=False)

def rydberg_1013_alignment(exp_postfix: str, pico_idx: int, timeout_control = {'use':False, 'timeout':1200}):
    NUM_POSITION_VAR = 9
    if pico_idx not in [3,4]:
        raise ValueError("pico_idx out of the range. Ranges are " + str([1,2,3,4]))
    VERTICAL = 1; HORIZONTAL = 2;
    align_axis = -1
    if pico_idx==3:
        align_axis = VERTICAL
    else:
        align_axis = HORIZONTAL
    exp_name_prefix = "RYDBERG-ALIGNMENT-1013"
    script_name = "rydberg_1013_alignment.mScript"

    config_name = "1013alignment.Config"
    config_path = exp.CONFIGURATION_DIR + config_name
    config_file = ConfigurationFile(config_path)

    # # Update configuration
    config_file.modify_parameter("REPETITIONS", "Reps:", str(5))
    config_file.modify_parameter("MAIN_OPTIONS", "Randomize Variations?", str(0))
    config_file.modify_parameter("MAIN_OPTIONS", "Repetition First Over Variation?", str(1))
    config_file.modify_parameter("PICOSCREW", "Control?", str(1))
    for idx in range(4):
        config_file.modify_parameter("PICOSCREW", f" Screw-{idx+1} Value:", "0")
    config_file.modify_parameter("PICOSCREW", f" Screw-{pico_idx} Value:", "integer_scan")
    
    for variable in config_file.config_param.variables:
        config_file.config_param.update_variable(variable.name, scan_type="Constant", scan_dimension=0)    
    config_file.config_param.update_variable("integer_scan", scan_type="Variable", scan_dimension=0, new_initial_values=[-200], new_final_values=[200])
    config_file.config_param.update_variable("resonance_scan", scan_type="Variable", scan_dimension=1, new_initial_values=[-5], new_final_values=[10])
    config_file.config_param.update_scan_dimension(0, range_index=0, variations=NUM_POSITION_VAR)
    config_file.config_param.update_scan_dimension(1, range_index=0, variations=11)
    config_file.save()

    exp.setPicoScrewHomes()

    # # Setup experiment details
    YEAR, MONTH, DAY = today()
    if align_axis==VERTICAL: axis_str = "VERTICAL"
    else: axis_str = "HORIZONTAL"
    exp_name = f"{exp_name_prefix}-{axis_str}-{exp_postfix}" 
    exp.open_configuration("\\ExperimentAutomation\\" + config_name)
    exp.open_master_script("\\ExperimentAutomation\\" + script_name)

    _calibration(exp=exp, config_file=config_file)

    exp.run_experiment(exp_name)
    
    # # Monitor experiment status
    experiment_monitoring(exp=exp, timeout_control=timeout_control)

    # Analyze the data
    data_analysis = da.DataAnalysis(YEAR, MONTH, DAY, exp_name, maximaLocs=analysis_locs.maximaLocs,
                            window=window, thresholds=thresholds, binnings=binnings, 
                            annotate_title = exp_name, annotate_note=" ")
    
    vertical_pos, horizontal_pos = data_analysis.analyze_mako_data(mako_idx=4, function = da.gaussian)
    if align_axis==VERTICAL: xkey = da.ah.nominal(vertical_pos).reshape(NUM_POSITION_VAR,11).mean(axis=1)
    else: xkey = da.ah.nominal(horizontal_pos).reshape(NUM_POSITION_VAR,11).mean(axis=1)
    analysis_result = data_analysis.analyze_data_2D(xkey0=xkey, function_d0=da.gaussian, function_d1=da.gaussian)
    optimal_field = analysis_result[1]
    
    if align_axis == VERTICAL:
        print_str = f"position ({optimal_field:.3S}, {horizontal_pos.mean():.3S})"
        target_position = (optimal_field.n, horizontal_pos.mean().n)
    else:
        print_str = f"position (vertical,horizontal) ({vertical_pos.mean():.3S}, {optimal_field:.3S})"
        target_position = (vertical_pos.mean().n, optimal_field.n)

    print(f"1013 lightshift for {exp_name} is {optimal_field:.3S}  with " + print_str)
    screws_position = exp.getPicoScrewPositions()
    exp.setPicoScrewPosition(3,screws_position[2], update=False)
    exp.setPicoScrewPosition(4,screws_position[3], update=False)
    move_beam_to_target(exp, mako_idx=4, pico_idx=(3,4), target_position=target_position, tolerance=0.3)
    sleep(1)
    exp.setPicoScrewHomes()
    exp.setPicoScrewPosition(3,position=0, update=False)
    exp.setPicoScrewPosition(4,position=0, update=False)

def rydberg_1013_alignment_DLS(exp_postfix: str, pico_idx: int, timeout_control = {'use':False, 'timeout':1200}):
    NUM_POSITION_VAR = 9
    if pico_idx not in [3,4]:
        raise ValueError("pico_idx out of the range. Ranges are " + str([1,2,3,4]))
    VERTICAL = 1; HORIZONTAL = 2;
    align_axis = -1
    if pico_idx==3:
        align_axis = VERTICAL
    else:
        align_axis = HORIZONTAL
    exp_name_prefix = "RYDBERG-ALIGNMENT-DLS-1013"
    script_name = "rydberg_1013_MWlightshift.mScript"

    config_name = "1013alignment_with_microwave.Config"
    config_path = exp.CONFIGURATION_DIR + config_name
    config_file = ConfigurationFile(config_path)

    # # Update configuration
    config_file.modify_parameter("REPETITIONS", "Reps:", str(4))
    config_file.modify_parameter("MAIN_OPTIONS", "Randomize Variations?", str(0))
    config_file.modify_parameter("MAIN_OPTIONS", "Repetition First Over Variation?", str(1))
    config_file.modify_parameter("PICOSCREW", "Control?", str(1))
    for idx in range(4):
        config_file.modify_parameter("PICOSCREW", f" Screw-{idx+1} Value:", "0")
    config_file.modify_parameter("PICOSCREW", f" Screw-{pico_idx} Value:", "integer_scan")
    
    for variable in config_file.config_param.variables:
        config_file.config_param.update_variable(variable.name, scan_type="Constant", scan_dimension=0)    
    config_file.config_param.update_variable("integer_scan", scan_type="Variable", scan_dimension=0, new_initial_values=[-200], new_final_values=[200])
    config_file.config_param.update_variable("resonance_scan", scan_type="Variable", scan_dimension=1, new_initial_values=[7.15], new_final_values=[7.23])
    config_file.config_param.update_scan_dimension(0, range_index=0, variations=NUM_POSITION_VAR)
    config_file.config_param.update_scan_dimension(1, range_index=0, variations=21)
    config_file.save()

    exp.setPicoScrewHomes()

    # # Setup experiment details
    YEAR, MONTH, DAY = today()
    if align_axis==VERTICAL: axis_str = "VERTICAL"
    else: axis_str = "HORIZONTAL"
    exp_name = f"{exp_name_prefix}-{axis_str}-{exp_postfix}" 
    exp.open_configuration("\\ExperimentAutomation\\" + config_name)
    exp.open_master_script("\\ExperimentAutomation\\" + script_name)

    _calibration(exp=exp, config_file=config_file)

    exp.run_experiment(exp_name)
    
    # # Monitor experiment status
    experiment_monitoring(exp=exp, timeout_control=timeout_control)

    # Analyze the data
    data_analysis = da.DataAnalysis(YEAR, MONTH, DAY, exp_name, maximaLocs=analysis_locs.maximaLocs,
                            window=window, thresholds=thresholds, binnings=binnings, 
                            annotate_title = exp_name, annotate_note=" ")
    
    vertical_pos, horizontal_pos = data_analysis.analyze_mako_data(mako_idx=4, function = da.gaussian)
    if align_axis==VERTICAL: xkey = da.ah.nominal(vertical_pos).reshape(NUM_POSITION_VAR,11).mean(axis=1)
    else: xkey = da.ah.nominal(horizontal_pos).reshape(NUM_POSITION_VAR,11).mean(axis=1)
    analysis_result = data_analysis.analyze_data_2D(xkey0=xkey, function_d0=da.gaussian, function_d1=da.gaussian)
    optimal_field = analysis_result[1]
    
    if align_axis == VERTICAL:
        print_str = f"position ({optimal_field:.3S}, {horizontal_pos.mean():.3S})"
        target_position = (optimal_field.n, horizontal_pos.mean().n)
    else:
        print_str = f"position (vertical,horizontal) ({vertical_pos.mean():.3S}, {optimal_field:.3S})"
        target_position = (vertical_pos.mean().n, optimal_field.n)

    print(f"1013 lightshift for {exp_name} is {optimal_field:.3S}  with " + print_str)
    screws_position = exp.getPicoScrewPositions()
    exp.setPicoScrewPosition(3,screws_position[2], update=False)
    exp.setPicoScrewPosition(4,screws_position[3], update=False)
    move_beam_to_target(exp, mako_idx=4, pico_idx=(3,4), target_position=target_position, tolerance=0.3)
    sleep(1)
    exp.setPicoScrewHomes()
    exp.setPicoScrewPosition(3,position=0, update=False)
    exp.setPicoScrewPosition(4,position=0, update=False)


def _calibration(exp:ExperimentProcedure ,config_file: ConfigurationFile):
    exp.setZynqOutput()
    analog_in_calibration(exp=exp, name = "prb_pwr")
    exp.save_all()
    sleep(2)
    analog_in_calibration(exp=exp, name = "op_pwr")
    exp.save_all()
    config_file.reopen()

def start_and_stop_camera(exp:ExperimentProcedure):
    exp.startMako(mako_idx=3)
    sleep(1)
    exp.startMako(mako_idx=4)
    sleep(1)
    exp.stopMako(mako_idx=3)
    sleep(1)
    exp.stopMako(mako_idx=4)
    sleep(1)

def zeroScrews(exp:ExperimentProcedure):
    exp.setPicoScrewHomes()
    exp.setPicoScrewPosition(1,0, update=False)
    exp.setPicoScrewPosition(2,0, update=False)
    exp.setPicoScrewPosition(3,0, update=False)
    exp.setPicoScrewPosition(4,0, update=False)

if __name__ == '__main__':
    zeroScrews(exp)

    # rydberg_420_lightshift(exp_postfix="2D-preAlignment")
    # exp.hardware_controller.restart_zynq_control()
    # rydberg_420_alignment(exp_postfix="2D", pico_idx=1)
    # exp.hardware_controller.restart_zynq_control()
    # rydberg_420_alignment(exp_postfix="2D", pico_idx=2)
    # exp.hardware_controller.restart_zynq_control()
    # rydberg_420_lightshift(exp_postfix="2D-postAlignment")
    # sleep(10)


    # rydberg_1013_lightshift(exp_postfix="2D-preAlignment")
    # exp.hardware_controller.restart_zynq_control()
    # rydberg_1013_alignment(exp_postfix="2D", pico_idx=3)
    # exp.hardware_controller.restart_zynq_control()
    # rydberg_1013_alignment(exp_postfix="2D", pico_idx=4)
    # exp.hardware_controller.restart_zynq_control()
    # rydberg_1013_lightshift(exp_postfix="2D-postAlignment")
    # sleep(10)

    rydberg_1013_lightshift_DLS(exp_postfix="2D-preAlignment")
    exp.hardware_controller.restart_zynq_control()
    rydberg_1013_alignment_DLS(exp_postfix="2D", pico_idx=3)
    exp.hardware_controller.restart_zynq_control()
    rydberg_1013_alignment_DLS(exp_postfix="2D", pico_idx=4)
    exp.hardware_controller.restart_zynq_control()
    rydberg_1013_lightshift_DLS(exp_postfix="2D-postAlignment")
    sleep(10)