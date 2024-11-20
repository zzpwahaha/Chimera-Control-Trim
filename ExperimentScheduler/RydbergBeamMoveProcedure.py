from ExperimentProcedure import *

def extract_beam_position_on_mako(exp : ExperimentProcedure, mako_idx:int, average_num = 5):
    if mako_idx not in [3,4]: return 
    exp.stopMako(mako_idx=mako_idx)
    t,l,w,h = exp.getMakoImageDimension(mako_idx)
    exposure = exp.getMakoFeatureValue(mako_idx, "ExposureTimeAbs", "double")[0]
    frame_rate = exp.getMakoFeatureValue(mako_idx, "AcquisitionFrameRateAbs", "double")[0]
    trigger_source = exp.getMakoFeatureValue(mako_idx, "TriggerSource", "string")
    horizontal_positions = []
    vertical_positions = []
    fit_results = []

    if mako_idx==3: # 420 blue camera
        exp.setTTL(name="ryd420trg", value=True)
        exp.setDAC(name='ryd420amp', value=-0.0135)
        exp.setMakoFeatureValue(mako_idx, "TriggerSource", "string", "FixedRate")
        exp.setMakoFeatureValue(mako_idx, "AcquisitionFrameRateAbs", "double", "5.0")
        exp.setMakoFeatureValue(mako_idx, "ExposureTimeAbs", "double", "16") # in us
        sleep(0.1)
        exp.startMako(mako_idx)
        sleep(5)

        id_avg=0
        while id_avg<average_num:
            id_avg += 1
            raw_img = exp.getMakoImage(mako_idx=mako_idx)
            if raw_img.shape[0] != h*w:
                id_avg += -1
                print("extract_beam_position_on_mako: wrong image size, let it go, raw_img.shape", raw_img.shape)
                continue
            img = raw_img.reshape(h,w)
            # horizontal 
            x,y = np.arange(w, dtype=float), img.sum(axis=0)
            punc, p, fit_result_str = da.ah.fit_data(x,y, fit_function=da.gaussian, use_unc=False)
            print(fit_result_str)
            horizontal_positions.append(punc[1])
            fit_results.append(punc)

            # vertical
            x,y = np.arange(h, dtype=float), img.sum(axis=1)
            punc, p, fit_result_str = da.ah.fit_data(x,y, fit_function=da.gaussian, use_unc=False)
            print(fit_result_str)
            vertical_positions.append(punc[1])
            fit_results.append(punc)

            sleep(0.5)
        exp.stopMako(mako_idx=mako_idx)
        exp.setTTL(name="ryd420trg", value=False)
        exp.setDAC(name='ryd420amp', value=0.15)

        
        # exp.getMakoFeatureValue(mako_idx=mako_idx)
    elif mako_idx==4: # 1013 blue camera
        exp.setTTL(name="ryd1013trg", value=True)
        exp.setDAC(name='ryd1013amp', value=0.45)
        exp.setMakoFeatureValue(mako_idx, "TriggerSource", "string", "FixedRate")
        exp.setMakoFeatureValue(mako_idx, "AcquisitionFrameRateAbs", "double", "5.0")
        exp.setMakoFeatureValue(mako_idx, "ExposureTimeAbs", "double", "1000") # in us
        sleep(0.1)
        exp.startMako(mako_idx)
        sleep(5)

        id_avg=0
        while id_avg<average_num:
            id_avg += 1
            raw_img = exp.getMakoImage(mako_idx=mako_idx)
            if raw_img.shape[0] != h*w:
                id_avg += -1
                print("extract_beam_position_on_mako: wrong image size, let it go, raw_img.shape", raw_img.shape)
                continue
            img = raw_img.reshape(h,w)
            # horizontal 
            x,y = np.arange(w, dtype=float), img.sum(axis=0)
            punc, p, fit_result_str = da.ah.fit_data(x,y, fit_function=da.gaussian, use_unc=False)
            print(fit_result_str)
            horizontal_positions.append(punc[1])
            fit_results.append(punc)


            # vertical
            x,y = np.arange(h, dtype=float), img.sum(axis=1)
            punc, p, fit_result_str = da.ah.fit_data(x,y, fit_function=da.gaussian, use_unc=False)
            print(fit_result_str)
            vertical_positions.append(punc[1])
            fit_results.append(punc)

            sleep(0.5)
        exp.stopMako(mako_idx=mako_idx)
        exp.setTTL(name="ryd1013trg", value=False)
        exp.setDAC(name='ryd1013amp', value=0.15)


    exp.setMakoFeatureValue(mako_idx, "TriggerSource", "string", trigger_source)
    exp.setMakoFeatureValue(mako_idx, "AcquisitionFrameRateAbs", "double", frame_rate)
    exp.setMakoFeatureValue(mako_idx, "ExposureTimeAbs", "double", exposure) # in us

    if (da.ah.std_dev(vertical_positions).mean()>10 or
        da.ah.std_dev(horizontal_positions).mean()>10 or 
        da.ah.nominal(fit_results)[:,0].mean()<1000 or
        da.ah.nominal(fit_results)[:,2].mean()>20):
        raise ValueError("Failed to extract beam position for its out ranged result")


    return np.array(vertical_positions), np.array(horizontal_positions)

def move_beam_to_target(exp: ExperimentProcedure, mako_idx: int, pico_idx: tuple, target_position: tuple, tolerance=0.1, max_iterations=50, gain=(-8, 8)):
    """
    Move the beam to the desired target position by adjusting two picoscrews (for vertical and horizontal axes), 
    using a user-specified scale factor (gain) for each axis. Stops if no further improvements can be made.
    Args:
        exp (ExperimentProcedure): The experiment procedure object.
        mako_idx (int): Index of the camera (either 3 or 4).
        pico_idx (tuple): Tuple of two ints, where pico_idx[0] is for the vertical axis and pico_idx[1] is for the horizontal axis.
        target_position (tuple): Desired (vertical, horizontal) beam position on the camera.
        tolerance (float): Allowable deviation from the target position.
        max_iterations (int): Maximum number of adjustments to prevent infinite loops.
        gain (tuple): Scale factor for the vertical and horizontal screw movements.
    Returns:
        bool: True if successful, False if maximum iterations were reached or no improvement is possible.
    """
    pico_idx_vertical, pico_idx_horizontal = pico_idx
    current_screw_position_vertical = exp.getPicoScrewPositions()[pico_idx_vertical - 1]
    current_screw_position_horizontal = exp.getPicoScrewPositions()[pico_idx_horizontal - 1]
    target_vertical, target_horizontal = target_position
    scale_factor_vertical, scale_factor_horizontal = gain

    # Set the new screw positions for both axes
    exp.setPicoScrewPosition(pico_idx_vertical, current_screw_position_vertical, update=False)
    exp.setPicoScrewPosition(pico_idx_horizontal, current_screw_position_horizontal, update=False)

    for iteration in range(max_iterations):
        # Get the current beam position
        vertical_positions, horizontal_positions = extract_beam_position_on_mako(exp, mako_idx)
        current_vertical = np.mean(da.ah.nominal(vertical_positions))
        current_horizontal = np.mean(da.ah.nominal(horizontal_positions))

        # Calculate the difference between current and target positions
        vertical_diff = target_vertical - current_vertical
        horizontal_diff = target_horizontal - current_horizontal
        # Check if the beam is within the tolerance range
        if abs(vertical_diff) < tolerance and abs(horizontal_diff) < tolerance:
            print(f"Beam moved to target position: ({current_vertical}, {current_horizontal}) in {iteration + 1} iterations.")
            return True
        
        # Calculate new screw positions based on the difference and scale factor (gain)
        new_screw_position_vertical = current_screw_position_vertical + int(scale_factor_vertical * vertical_diff)
        new_screw_position_horizontal = current_screw_position_horizontal + int(scale_factor_horizontal * horizontal_diff)
        # Check if the screw positions haven't changed; if so, exit early
        if new_screw_position_vertical == current_screw_position_vertical and new_screw_position_horizontal == current_screw_position_horizontal:
            print("No further improvement can be made; screw positions are no longer changing.")
            return True
        
        # Set the new screw positions for both axes
        exp.setPicoScrewPosition(pico_idx_vertical, new_screw_position_vertical)
        exp.setPicoScrewPosition(pico_idx_horizontal, new_screw_position_horizontal)
        
        # Update current screw positions
        current_screw_position_vertical = new_screw_position_vertical
        current_screw_position_horizontal = new_screw_position_horizontal

        # Wait for the system to stabilize before re-measuring
        sleep(1)
    
    print(f"Failed to move the beam to target within {max_iterations} iterations.")
    return False


RYDBERG_BEAM_420_POSITION = (16.21,28.80) #(15.75,27.46) #(16.12,28.30) #(14.81,28.74) #(16.48,28.57) #(16.95,28.71)
RYDBERG_BEAM_1013_POSITION = (32.31, 26.02) #(31.88, 27.06) #(31.31,26.33) #(31.89,26.19) #(32.02,25.56) #(33.65,26.38)

if __name__=="__main__":
    exp = ExperimentProcedure()
    # move_beam_to_target(exp, mako_idx=3, pico_idx=(1,2), target_position=(74,68.7), tolerance=0.3,max_iterations=50, gain=(-8, 8))
    move_beam_to_target(exp, mako_idx=3, pico_idx=(1,2), target_position=RYDBERG_BEAM_420_POSITION, tolerance=0.1,max_iterations=50, gain=(-8, 8))
    print("asd")

    move_beam_to_target(exp, mako_idx=4, pico_idx=(3,4), target_position=RYDBERG_BEAM_1013_POSITION, tolerance=0.1,max_iterations=50, gain=(-8, 8))
    print("asd")
