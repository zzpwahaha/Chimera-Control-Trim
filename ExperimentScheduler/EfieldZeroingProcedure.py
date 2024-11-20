from ExperimentProcedure import *
from RydbergBeamMoveProcedure import move_beam_to_target, RYDBERG_BEAM_420_POSITION, RYDBERG_BEAM_1013_POSITION

def EfieldCalibrationProcedure():
    YEAR, MONTH, DAY = today()
    exp = ExperimentProcedure()
    config_name = "EfieldCalibration.Config"
    config_path = exp.CONFIGURATION_DIR + config_name

    window = [0, 0, 200, 30]
    thresholds = 65
    binnings = np.linspace(0, 240, 241)
    analysis_locs = da.DataAnalysis(year='2024', month='August', day='26', data_name='data_1', 
                                    window=window, thresholds=70, binnings=binnings)

    # for bias E x range = [-0.5, 1] 16, E y range = [-1.2, 0.0] 16, E z range = [-1., 1.0] 16
    config_file = ConfigurationFile(config_path)
    config_file.modify_parameter("REPETITIONS", "Reps:", str(4))
    for variable in config_file.config_param.variables:
        config_file.config_param.update_variable(variable.name, scan_type="Constant", scan_dimension=0)    
    config_file.config_param.update_scan_dimension(0, range_index=0, variations=5)
    config_file.config_param.update_scan_dimension(1, range_index=0, variations=15)

    config_file.config_param.update_variable("time_scan_us", constant_value = 0.18)
    config_file.config_param.update_variable("ryd_420_amplitude", constant_value = 0.1)
    config_file.config_param.update_variable("ryd_1013_amplitude", constant_value = 0.45)

    config_file.config_param.update_variable("bias_e_x", scan_type="Variable", new_initial_values=[0], new_final_values=[2])
    config_file.config_param.update_variable("bias_e_y", scan_type="Constant", new_initial_values=[-1.5], new_final_values=[0.5])
    config_file.config_param.update_variable("bias_e_z", scan_type="Constant", new_initial_values=[-1.0], new_final_values=[1.0])
    config_file.config_param.update_variable("resonance_scan", scan_type="Variable", new_initial_values=[73], new_final_values=[87])

    # exp.hardware_controller.restart_zynq_control()
    # sleep(1)
    recenter_beams(exp)
    _calibration(exp,config_file)

    # E_x
    YEAR, MONTH, DAY = today()
    config_file.config_param.update_variable("resonance_scan", scan_dimension=1, scan_type="Variable")
    config_file.config_param.update_variable("bias_e_x", scan_dimension=0, scan_type="Variable")
    config_file.config_param.update_variable("bias_e_y", scan_dimension=0, scan_type="Constant")
    config_file.config_param.update_variable("bias_e_z", scan_dimension=0, scan_type="Constant")
    config_file.save()

    exp_name = "EFIELD-X"
    exp.open_configuration("\\ExperimentAutomation\\" + config_name)
    exp.run_experiment(exp_name)
    sleep(1)
    experiment_monitoring(exp=exp, timeout_control={'use':False, 'timeout':600})


    data_analysis = da.DataAnalysis(YEAR, MONTH, DAY, exp_name, maximaLocs=analysis_locs.maximaLocs,
                            window=window, thresholds=thresholds, binnings=binnings, 
                            annotate_title = exp_name, annotate_note=" ")
    analysis_result = data_analysis.analyze_data_2D(function_d0 = da.Quadratic, function_d1 = da.gaussian)
    optimal_field = analysis_result[1]
    print(f"Optimal field for {exp_name} is {optimal_field:.3S} ")

    exp.hardware_controller.restart_zynq_control()
    sleep(1)
    recenter_beams(exp)
    _calibration(exp,config_file)

    # E_y
    YEAR, MONTH, DAY = today()
    config_file.config_param.update_variable("resonance_scan", scan_dimension=1, scan_type="Variable")
    config_file.config_param.update_variable("bias_e_x", scan_dimension=0, scan_type="Constant")
    config_file.config_param.update_variable("bias_e_y", scan_dimension=0, scan_type="Variable")
    config_file.config_param.update_variable("bias_e_z", scan_dimension=0, scan_type="Constant")
    config_file.config_param.update_variable("bias_e_x", constant_value = round(optimal_field.n, 3))
    config_file.save()

    exp_name = "EFIELD-Y"
    exp.open_configuration("\\ExperimentAutomation\\" + config_name)
    exp.run_experiment(exp_name)
    sleep(1)
    experiment_monitoring(exp=exp, timeout_control={'use':False, 'timeout':600})

    data_analysis = da.DataAnalysis(YEAR, MONTH, DAY, exp_name, maximaLocs=analysis_locs.maximaLocs,
                            window=window, thresholds=thresholds, binnings=binnings, 
                            annotate_title = exp_name, annotate_note=" ")
    analysis_result = data_analysis.analyze_data_2D(function_d0 = da.Quadratic, function_d1 = da.gaussian)
    optimal_field = analysis_result[1]
    print(f"Optimal field for {exp_name} is {optimal_field:.3S} ")

    exp.hardware_controller.restart_zynq_control()
    sleep(1)
    recenter_beams(exp)
    _calibration(exp,config_file)

    # E_z
    YEAR, MONTH, DAY = today()
    config_file.config_param.update_variable("resonance_scan", scan_dimension=1, scan_type="Variable")
    config_file.config_param.update_variable("bias_e_x", scan_dimension=0, scan_type="Constant")
    config_file.config_param.update_variable("bias_e_y", scan_dimension=0, scan_type="Constant")
    config_file.config_param.update_variable("bias_e_z", scan_dimension=0, scan_type="Variable")
    config_file.config_param.update_variable("bias_e_y", constant_value = round(optimal_field.n, 3))
    config_file.save()

    exp_name = "EFIELD-Z"
    exp.open_configuration("\\ExperimentAutomation\\" + config_name)
    exp.run_experiment(exp_name)
    sleep(1)
    experiment_monitoring(exp=exp, timeout_control={'use':False, 'timeout':600})

    data_analysis = da.DataAnalysis(YEAR, MONTH, DAY, exp_name, maximaLocs=analysis_locs.maximaLocs,
                            window=window, thresholds=thresholds, binnings=binnings, 
                            annotate_title = exp_name, annotate_note=" ")
    analysis_result = data_analysis.analyze_data_2D()
    optimal_field = analysis_result[1]
    print(f"Optimal field for {exp_name} is {optimal_field:.3S} ")

def recenter_beams(exp: ExperimentProcedure):
    exp.setDAC()
    sleep(1)
    exp.setDDS()
    move_beam_to_target(exp=exp, mako_idx=3, pico_idx=(1,2), target_position=RYDBERG_BEAM_420_POSITION, tolerance=0.2)
    sleep(1)
    move_beam_to_target(exp=exp, mako_idx=4, pico_idx=(3,4), target_position=RYDBERG_BEAM_1013_POSITION, tolerance=0.2)
    sleep(1)

def _calibration(exp: ExperimentProcedure, config_file: ConfigurationFile):
    exp.setZynqOutput()
    analog_in_calibration(exp=exp, name = "prb_pwr")
    exp.save_all()
    config_file.reopen()

if __name__=='__main__':
    EfieldCalibrationProcedure()