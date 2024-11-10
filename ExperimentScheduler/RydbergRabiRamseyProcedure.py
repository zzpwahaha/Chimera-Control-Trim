import numpy as np
from ExperimentProcedure import *
from ExperimentProcedure import experiment_monitoring, analog_in_calibration_monitoring
from RydbergBeamMoveProcedure import move_beam_to_target, RYDBERG_BEAM_420_POSITION, RYDBERG_BEAM_1013_POSITION
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

exp.open_configuration("\\ExperimentAutomation\\" + config_name)

# analysis grid
window = [0, 0, 200, 30]
thresholds = 65
binnings = np.linspace(0, 240, 241)
analysis_locs = da.DataAnalysis(year='2024', month='August', day='26', data_name='data_1', 
                                window=window, thresholds=70, binnings=binnings)


def resonace_scan(exp_idx, timeout_control = {'use':False, 'timeout':600}):
    script_name = "Calibration_rydberg_420_1013_excitation.mScript"

    # Update configuration
    config_file.modify_parameter("REPETITIONS", "Reps:", str(4))
    for variable in config_file.config_param.variables:
        config_file.config_param.update_variable(variable.name, scan_type="Constant", scan_dimension=0)    
    config_file.config_param.update_scan_dimension(0, new_ranges=[ScanRange(index=0,left_inclusive=True, right_inclusive=True, variations=33)])
    config_file.config_param.update_variable("resonance_scan", scan_type="Variable", new_initial_values=[73], new_final_values=[87])
    config_file.config_param.update_variable("time_scan_us", constant_value = 0.15)
    config_file.save()
    
    # Setup experiment details
    YEAR, MONTH, DAY = today()
    exp_name = f"RESONANCE-SCAN-{exp_idx}"
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

    # Update configuration with the optimal field
    config_file.config_param.update_variable("resonance_scan", constant_value = round(optimal_field.n, 3))
    config_file.save()
    exp.open_configuration("\\ExperimentAutomation\\" + config_name)
    return  fit_fail

def rabi_scan(exp_idx, timeout_control = {'use':False, 'timeout':600}):
    script_name = "Calibration_rydberg_420_1013_excitation.mScript"
    config_file.modify_parameter("REPETITIONS", "Reps:", str(5))
    for variable in config_file.config_param.variables:
        config_file.config_param.update_variable(variable.name, scan_type="Constant", scan_dimension=0)    
    # config_file.config_param.update_scan_dimension(0, new_ranges=[
    #     ScanRange(index=0,left_inclusive=True, right_inclusive=True, variations=21),
    #     ScanRange(index=1,left_inclusive=True, right_inclusive=True, variations=11),
    #     ScanRange(index=2,left_inclusive=True, right_inclusive=True, variations=11),
    #     ScanRange(index=2,left_inclusive=True, right_inclusive=True, variations=11),
    #     ScanRange(index=2,left_inclusive=True, right_inclusive=True, variations=11)])
    # config_file.config_param.update_variable("time_scan_us", scan_type="Variable", new_initial_values=[0.01,1.5,3.0,4.5,6.0], new_final_values=[0.41,1.8,3.3,4.8,6.3])
    
    # config_file.config_param.update_scan_dimension(0, new_ranges=[
    #     ScanRange(index=0,left_inclusive=True, right_inclusive=True, variations=21),
    #     ScanRange(index=1,left_inclusive=True, right_inclusive=True, variations=11),
    #     ScanRange(index=2,left_inclusive=True, right_inclusive=True, variations=11)])
    # config_file.config_param.update_variable("time_scan_us", scan_type="Variable", new_initial_values=[0.01,1.5,3.0], new_final_values=[0.41,1.8,3.3])

    # config_file.config_param.update_scan_dimension(0, new_ranges=[
    #     ScanRange(index=0,left_inclusive=True, right_inclusive=True, variations=16),
    #     ScanRange(index=1,left_inclusive=True, right_inclusive=True, variations=11),
    #     ScanRange(index=2,left_inclusive=True, right_inclusive=True, variations=11),
    #     ScanRange(index=2,left_inclusive=True, right_inclusive=True, variations=11)
    #     ])
    # config_file.config_param.update_variable("time_scan_us", scan_type="Variable", 
    #                                          new_initial_values=[0.01,1.5,3.0,4.5], 
    #                                          new_final_values=[0.61,1.9,3.4,4.9])

    config_file.config_param.update_scan_dimension(0, new_ranges=[
        ScanRange(index=0,left_inclusive=True, right_inclusive=True, variations=16),
        ScanRange(index=1,left_inclusive=True, right_inclusive=True, variations=11),
        ScanRange(index=2,left_inclusive=True, right_inclusive=True, variations=11)
        ])
    config_file.config_param.update_variable("time_scan_us", scan_type="Variable", 
                                             new_initial_values=[0.01,2.2,4.5], 
                                             new_final_values=[0.61,2.6,4.9])


    config_file.save()
    
    YEAR, MONTH, DAY = today()
    exp_name = f"RABI-SCAN-{exp_idx}"
    exp.open_configuration("\\ExperimentAutomation\\" + config_name)
    exp.open_master_script("\\ExperimentAutomation\\" + script_name)
    exp.run_experiment(exp_name)

    # Monitor experiment status
    experiment_monitoring(exp=exp, timeout_control=timeout_control)

    data_analysis = da.DataAnalysis(YEAR, MONTH, DAY, exp_name, maximaLocs=analysis_locs.maximaLocs,
                            window=window, thresholds=thresholds, binnings=binnings, 
                            annotate_title = exp_name, annotate_note=" ")
    return

def pi_pi2_time_scan(exp_idx, timeout_control = {'use':False, 'timeout':600}):
    script_name = "Calibration_rydberg_420_1013_excitation.mScript"

    # Update configuration
    config_file.modify_parameter("REPETITIONS", "Reps:", str(5))
    for variable in config_file.config_param.variables:
        config_file.config_param.update_variable(variable.name, scan_type="Constant", scan_dimension=0)    
    config_file.config_param.update_scan_dimension(0, new_ranges=[ScanRange(index=0,left_inclusive=True, right_inclusive=True, variations=26)])
    config_file.config_param.update_variable("time_scan_us", scan_type="Variable", new_initial_values=[0.01], new_final_values=[0.26])
    config_file.save()
    
    # Setup experiment details
    YEAR, MONTH, DAY = today()
    exp_name = f"PI-TIME-SCAN-{exp_idx}"
    exp.open_configuration("\\ExperimentAutomation\\" + config_name)
    exp.open_master_script("\\ExperimentAutomation\\" + script_name)
    exp.run_experiment(exp_name)
    
    # Monitor experiment status
    experiment_monitoring(exp=exp, timeout_control=timeout_control)

    # Analyze the data
    # rabi_analysis = da.DataAnalysis(YEAR, MONTH, DAY, exp_name, maximaLocs=analysis_locs.maximaLocs,
    #                         window=window, thresholds=thresholds, binnings=binnings, 
    #                         annotate_title = f"RABI-SCAN-{exp_idx}", annotate_note=" ")
    # rabi_guess = rabi_analysis.analyze_data(function = da.decaying_cos)
    # rabi_guess = da.ah.nominal(rabi_guess)
    RABI_GUESS = [1,5,4,0,0.5]
    data_analysis = da.DataAnalysis(YEAR, MONTH, DAY, exp_name, maximaLocs=analysis_locs.maximaLocs,
                            window=window, thresholds=thresholds, binnings=binnings, 
                            annotate_title = exp_name, annotate_note=" ")
    fit_result = data_analysis.analyze_data(function = da.decaying_cos, p0=RABI_GUESS)
    punc, p = fit_result, da.ah.nominal(fit_result)
    
    # find pi time
    angle = np.pi-p[3]
    angle = np.fmod(angle, 2 * np.pi)
    if angle < 0:
        angle += 2 * np.pi
    angle = da.ah.unc.ufloat(angle, punc[3].std_dev)
    t_pi = angle / (2*np.pi*punc[2])
    if  t_pi<0.01 or t_pi>0.5:
        print(f"Faulty fitted pi time {t_pi:.3S}")
    else:
        print(f"Good fitted pi time {t_pi:.3S}")
        config_file.config_param.update_variable("ryd_pi_us", constant_value = round(t_pi.n, 2))


    # find pi2 time
    angle = da.ah.unp.arccos((0.5-punc[4])/punc[0])-punc[3]
    angle_n, angle_s = angle.nominal_value, angle.std_dev
    angle_n = np.fmod(angle_n, 2 * np.pi)
    if angle_n < 0:
        angle_n += 2 * np.pi
    angle = da.ah.unc.ufloat(angle_n, angle_s)
    t_pi2 = angle / (2*np.pi*punc[2])
    if  t_pi2<0.01 or t_pi2>0.5:
        raise ValueError(f"Faulty fitted pi2 time {t_pi2:.3S}")
    else:
        print(f"Good fitted pi2 time {t_pi2:.3S}")
        config_file.config_param.update_variable("ryd_pi2_us", constant_value = round(t_pi2.n, 2))

    # Update configuration with the optimal field
    config_file.save()
    exp.open_configuration("\\ExperimentAutomation\\" + config_name)

def ryd_1013_mw_lightshift_scan(exp_idx, timeout_control = {'use':False, 'timeout':600}):
    script_name = "rydberg_1013_MWlighshift.mScript"
    MW_COMMAND = 'rb87_hfsplitting+mw_resonance+7.215-amplitude_scan*0.18'

    config_name_mw = "Microwave.Config"
    config_path_mw = exp.CONFIGURATION_DIR + config_name_mw
    config_file = ConfigurationFile(config_path_mw)

    config_file.modify_parameter("REPETITIONS", "Reps:", str(3))
    config_file.modify_parameter("MAIN_OPTIONS", "Randomize Variations?", str(1))
    config_file.modify_parameter("MAIN_OPTIONS", "Repetition First Over Variation?", str(0))
    mw_section = config_file.get_section("MICROWAVE_SYSTEM")
    mw_section.mw_lists[1].parameters['Freq:'] = MW_COMMAND

    for variable in config_file.config_param.variables:
        config_file.config_param.update_variable(variable.name, scan_type="Constant", scan_dimension=0)    
    config_file.config_param.update_variable("mw_resonance", scan_type="Variable", scan_dimension=1, new_initial_values=[-0.02], new_final_values=[0.02])
    config_file.config_param.update_variable("amplitude_scan", scan_type="Variable", scan_dimension=0, new_initial_values=[0], new_final_values=[0.45])
    config_file.config_param.update_scan_dimension(0, range_index=0, variations=3)
    config_file.config_param.update_scan_dimension(1, range_index=0, variations=21)
    config_file.save()
    
    # Setup experiment details
    YEAR, MONTH, DAY = today()
    exp_name = f"RYD1013-MW-LS-{exp_idx}"
    exp.open_configuration("\\ExperimentAutomation\\" + config_name_mw)
    exp.open_master_script("\\ExperimentAutomation\\" + script_name)
    exp.run_experiment(exp_name)

    # Monitor experiment status
    experiment_monitoring(exp=exp, timeout_control=timeout_control)

    exp.open_configuration("\\ExperimentAutomation\\" + config_name)

    data_analysis = da.DataAnalysis(YEAR, MONTH, DAY, exp_name, maximaLocs=analysis_locs.maximaLocs,
                            window=window, thresholds=thresholds, binnings=binnings, 
                            annotate_title = exp_name, annotate_note= MW_COMMAND)
    return

def ramsey_scan(exp_idx, timeout_control = {'use':False, 'timeout':600}):
    script_name = "Calibration_rydberg_420_1013_Ramsey.mScript"
    config_file.modify_parameter("REPETITIONS", "Reps:", str(5))
    for variable in config_file.config_param.variables:
        config_file.config_param.update_variable(variable.name, scan_type="Constant", scan_dimension=0)    
    config_file.config_param.update_scan_dimension(0, new_ranges=[
        ScanRange(index=0,left_inclusive=True, right_inclusive=True, variations=11),
        ScanRange(index=1,left_inclusive=True, right_inclusive=True, variations=11),
        ScanRange(index=2,left_inclusive=True, right_inclusive=True, variations=11)])
    config_file.config_param.update_variable("time_scan_us", scan_type="Variable", new_initial_values=[0.5,1.5,2.5], new_final_values=[1,2,3])
    config_file.save()
    
    YEAR, MONTH, DAY = today()
    exp_name = f"RAMSEY-SCAN-420-BEAM-OFF-{exp_idx}"
    exp.open_configuration("\\ExperimentAutomation\\" + config_name)
    exp.open_master_script("\\ExperimentAutomation\\" + script_name)
    exp.run_experiment(exp_name)

    # Monitor experiment status
    experiment_monitoring(exp=exp, timeout_control=timeout_control)

    data_analysis = da.DataAnalysis(YEAR, MONTH, DAY, exp_name, maximaLocs=analysis_locs.maximaLocs,
                            window=window, thresholds=thresholds, binnings=binnings, 
                            annotate_title = exp_name, annotate_note=" ")
    return
    
def ramsey_scan_echo(exp_idx, timeout_control = {'use':False, 'timeout':600}):
    script_name = "Calibration_rydberg_420_1013_Ramsey_echo.mScript"
    config_file.modify_parameter("REPETITIONS", "Reps:", str(5))
    for variable in config_file.config_param.variables:
        config_file.config_param.update_variable(variable.name, scan_type="Constant", scan_dimension=0)    
    config_file.config_param.update_scan_dimension(0, new_ranges=[
        ScanRange(index=0,left_inclusive=True, right_inclusive=True, variations=21),])
    config_file.config_param.update_variable("time_scan_us", scan_type="Variable", new_initial_values=[0.5], new_final_values=[7.5])
    config_file.save()
    
    YEAR, MONTH, DAY = today()
    exp_name = f"RAMSEY-SCAN-420-BEAM-OFF-ECHO-{exp_idx}"
    exp.open_configuration("\\ExperimentAutomation\\" + config_name)
    exp.open_master_script("\\ExperimentAutomation\\" + script_name)
    exp.run_experiment(exp_name)

    # Monitor experiment status
    experiment_monitoring(exp=exp, timeout_control=timeout_control)

    data_analysis = da.DataAnalysis(YEAR, MONTH, DAY, exp_name, maximaLocs=analysis_locs.maximaLocs,
                            window=window, thresholds=thresholds, binnings=binnings, 
                            annotate_title = exp_name, annotate_note=" ")
    return

def ramsey_scan_bothOff(exp_idx, timeout_control = {'use':False, 'timeout':600}):
    script_name = "Calibration_rydberg_420_1013_Ramsey_bothOff.mScript"
    config_file.modify_parameter("REPETITIONS", "Reps:", str(7))
    for variable in config_file.config_param.variables:
        config_file.config_param.update_variable(variable.name, scan_type="Constant", scan_dimension=0)    
    config_file.config_param.update_variable("time_scan_us", scan_type="Variable", new_initial_values=[0.5,1.5,2.5], new_final_values=[1,2,3])
    config_file.config_param.update_scan_dimension(0, new_ranges=[
        ScanRange(index=0,left_inclusive=True, right_inclusive=True, variations=11),
        ScanRange(index=1,left_inclusive=True, right_inclusive=True, variations=11),
        ScanRange(index=2,left_inclusive=True, right_inclusive=True, variations=11)])
    config_file.save()
    
    YEAR, MONTH, DAY = today()
    exp_name = f"RAMSEY-SCAN-BOTH-BEAM-OFF-{exp_idx}"
    exp.open_configuration("\\ExperimentAutomation\\" + config_name)
    exp.open_master_script("\\ExperimentAutomation\\" + script_name)
    exp.run_experiment(exp_name)
    
    # Monitor experiment status
    experiment_monitoring(exp=exp, timeout_control=timeout_control)

    data_analysis = da.DataAnalysis(YEAR, MONTH, DAY, exp_name, maximaLocs=analysis_locs.maximaLocs,
                            window=window, thresholds=thresholds, binnings=binnings, 
                            annotate_title = exp_name, annotate_note=" ")
    return

def _calibration():
    exp.setZynqOutput()
    analog_in_calibration(exp=exp, name = "prb_pwr")
    exp.save_all()
    config_file.reopen()
    sleep(1)


def recenter_beams():
    exp.setDAC()
    sleep(1)
    exp.setDDS()
    move_beam_to_target(exp=exp, mako_idx=3, pico_idx=(1,2), target_position=RYDBERG_BEAM_420_POSITION, tolerance=0.2)
    sleep(1)
    move_beam_to_target(exp=exp, mako_idx=4, pico_idx=(3,4), target_position=RYDBERG_BEAM_1013_POSITION, tolerance=0.2)
    sleep(1)
    exp.save_all()
    config_file.reopen()
    sleep(1)

def calibration(exp_idx):
    try:
        _calibration()
        recenter_beams()
        resonace_scan(exp_idx=exp_idx, timeout_control = {'use':True, 'timeout':900})
    except Exception as e:
        print(e)
        exp.hardware_controller.restart_zynq_control()
        # calibration(exp_idx)
        return
    try:
        exp.hardware_controller.restart_zynq_control()
        _calibration()
        recenter_beams()
        rabi_scan(exp_idx=exp_idx, timeout_control = {'use':True, 'timeout':1200}) #1500
        sleep(3)
        exp.hardware_controller.restart_zynq_control()
        sleep(3)
        pi_pi2_time_scan(exp_idx=exp_idx, timeout_control = {'use':True, 'timeout':1200})
        sleep(3)

        _calibration()
        recenter_beams()
        exp.hardware_controller.restart_zynq_control()
        ramsey_scan(exp_idx=exp_idx, timeout_control = {'use':True, 'timeout':1200})
        sleep(3)

        _calibration()
        recenter_beams()
        exp.hardware_controller.restart_zynq_control()
        ramsey_scan_echo(exp_idx=exp_idx, timeout_control = {'use':True, 'timeout':1200})
        sleep(3)

        recenter_beams()
        exp.hardware_controller.restart_zynq_control()
        ryd_1013_mw_lightshift_scan(exp_idx=exp_idx, timeout_control = {'use':True, 'timeout':1200})
        sleep(3)

        # _calibration()
        # exp.hardware_controller.restart_zynq_control()
        # ramsey_scan_bothOff(exp_idx=exp_idx, timeout_control = {'use':True, 'timeout':1200})
    except Exception as e:
        print(e)
        exp.hardware_controller.restart_zynq_control()
        return


def procedure():
    for idx in np.arange(50):
        # if idx<=1: continue
        print(f"Running experiment sets number {idx}")
        if idx != 0:
            exp.hardware_controller.restart_zynq_control()
        calibration(idx)


def test(exp_idx):
    resonace_scan(exp_idx=exp_idx)
    rabi_scan(exp_idx=exp_idx)
    ramsey_scan(exp_idx=exp_idx)
    ramsey_scan_bothOff(exp_idx=exp_idx)
    

if __name__=='__main__':
    procedure()

    # ryd_1013_mw_lightshift_scan(0)
    # test(0)
