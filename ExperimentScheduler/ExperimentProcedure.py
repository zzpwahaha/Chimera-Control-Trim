import sys
from datetime import datetime
from time import sleep
import time
import numpy as np
import struct

from ZynqController.ZynqStarter import ZynqStarter
from ZynqController.TCPClient import TCPClient
from Logging.PrintLogger import Logger
import DataProcess.DataAnalyzer as da
from ConfigurationManager.ConfigurationFile import ConfigurationFile
from ConfigurationManager.ConfigurationScanParameter.ScanRange import ScanRange


class ExperimentProcedure:
    CONFIGURATION_DIR = "C:/Chimera/Chimera-Cryo/Configurations/ExperimentAutomation/"
    CHIMERA_HOST = 'localhost'
    CHIMERA_PORT = 8888
    ZYNQ_HOST = '10.10.0.2'
    ZYNQ_PORT = 8080
    SYNACCESS_HOST_ZYNQ = '10.10.0.100'
    SYNACCESS_HOST_COIL = '10.10.0.101'

    def __init__(self):
        # Set up logging
        current_datetime = datetime.now()
        formatted_datetime_dash = current_datetime.strftime("%Y-%m-%d_%H-%M-%S")
        self.logger = Logger(f'./log/{formatted_datetime_dash}.log')
        sys.stdout = self.logger
        
        # Initialize TCP client and hardware controller
        self.chimera_client = TCPClient(self.CHIMERA_HOST, self.CHIMERA_PORT)
        self.hardware_controller = ZynqStarter(self.ZYNQ_HOST, self.ZYNQ_PORT, 
                                               self.SYNACCESS_HOST_ZYNQ, self.SYNACCESS_HOST_COIL)

        # Establish connection with Chimera
        self.chimera_client.connect()

    def _chimera_command(self, command: str, bufsize = 4096, data_type = 'double') :
        result = self.chimera_client.query(command, bufsize)
        result_message, result_data = self.decode_reply(result, data_type)
        print(result_message)
        return (not self._judge_message_failure(result_message)), result_data
        
    def chimera_command(self, command: str) -> bool:
        return self._chimera_command(command)[0]

    def chimera_bool_command(self, command: str) -> bool:
        result = self.chimera_client.query(command)
        result_message, _ = self.decode_reply(result)
        self._judge_message_failure(result_message)
        if 'TRUE' in result_message:
            return True
        elif 'FALSE' in result_message:
            return False
        else:
            print(f"Unexpected response received: {result}")

    def chimera_data_command(self, command: str, bufsize=4096, data_type = 'double') -> bool:
        return self._chimera_command(command, bufsize, data_type)[1]

    @staticmethod
    def decode_reply(result_message: bytes, data_type = 'double'):
        # Decode the message
        if b'$' not in result_message:
            return result_message.decode('utf-8'), np.array([])
        # Find the delimiter '$'        
        delimiter_index = result_message.index(b'$')
        message = result_message[:delimiter_index].decode('utf-8')  # Get the string message
        # Get the size of the vector (8 bytes), size of the size_t in Chimera MessageConsumer::compileReply
        size_bytes = result_message[delimiter_index + 1:delimiter_index + 1 + 8]
        size = struct.unpack('<Q', size_bytes)[0]  # Unpack as unsigned int, little endian for intel x86 and AMD
        # Get the vector data
        start_of_data = delimiter_index + 1 + 8
        if data_type=='double':
            vector_data = result_message[start_of_data:start_of_data + size * 8]  # 8 bytes per double
            print(f"len(vector_data): {len(vector_data)}")
            vector = struct.unpack(f'<{size}d', vector_data)  # Unpack as double values
        elif data_type == 'int':
            vector_data = result_message[start_of_data:start_of_data + size * 4]  # 4 bytes per int
            vector = struct.unpack(f'<{size}i', vector_data)  # Unpack as int values
        elif data_type == 'char' or data_type == 'string':
            vector_data = result_message[start_of_data:start_of_data + size * 1]  # 1 bytes per char
            vector = struct.unpack(f'<{size}s', vector_data)  # Unpack as char values
        else:
            raise ValueError("Unknown type of data_type in decode_reply")
        return message, np.array(vector)

    @staticmethod
    def _judge_message_failure(result_message: str) -> bool:
        failure_keywords = ["Error", "ERROR", "error"]
        if any(keyword in result_message for keyword in failure_keywords):
            print(f"Command failed: {result_message}")
            return True
        else:
            print(f"Command succeeded: {result_message}")
            return False

    def run_experiment(self, name: str):
        if self.save_all():
            self.chimera_command(f"Start-Experiment ${name}")

    def abort_experiment(self, save=True, file_name="placeholder"):
        action = 'save' if save else 'delete'
        if not save and file_name=="placeholder":
            print('Failed to abort and delete data: no valid file name provided.')
            return
        self.chimera_command(f"Abort-Experiment ${action} ${file_name}")

    def open_configuration(self, config_path_name: str):
        self.chimera_command(f"Open-Configuration ${config_path_name}")

    def open_master_script(self, script_path_name: str):
        self.chimera_command(f"Open-Master-Script ${script_path_name}")

    def save_all(self):
        return self.chimera_command(f"Save-All")

    def is_experiment_running(self) -> bool:
        return self.chimera_bool_command("Is-Experiment-Running?")
    
    def run_calibration(self, name: str):
        if self.save_all():
            self.chimera_command(f"Start-Calibration ${name}")

    def is_calibration_running(self) -> bool:
        return self.chimera_bool_command("Is-Calibration-Running?")

    def setStaticDDS(self, ddsfreq : float, channel : int):
        return self.chimera_command(f"Set-Static-DDS ${ddsfreq:.6f} ${channel:d}") # Hz resoultion, in MHz unit

    def setTTL(self, name:str, value:bool):
        return self.chimera_command(f"Set-TTL ${name}${int(value)}")
    
    def setDAC(self, name:str= "", value:float=0.0):
        return self.chimera_command(f"Set-DAC ${name}${value:.4f}")
    
    def setDDS(self):
        return self.chimera_command(f"Set-DDS")

    def setOL(self):
        return self.chimera_command(f"Set-OL")

    def setZynqOutput(self):
        self.setDAC()
        time.sleep(1)
        self.setDDS()
        time.sleep(1)
        self.setOL()
        time.sleep(1)

    def startMako(self, mako_idx: int):
        if mako_idx not in [1,2,3,4]:
            print("mako_idx out of the range. Ranges are " + str([1,2,3,4]))
            return
        return self.chimera_data_command(f"Start-MAKO $mako{mako_idx:d}") 

    def stopMako(self, mako_idx: int):
        if mako_idx not in [1,2,3,4]:
            print("mako_idx out of the range. Ranges are " + str([1,2,3,4]))
            return
        return self.chimera_data_command(f"Stop-MAKO $mako{mako_idx:d}") 

    def getMakoImage(self, mako_idx: int):
        if mako_idx not in [1,2,3,4]:
            print("mako_idx out of the range. Ranges are " + str([1,2,3,4]))
            return
        return self.chimera_data_command(f"Get-MAKO-Image $mako{mako_idx:d}", bufsize=65536*8) # .reshape(height, width)

    def getMakoImageDimension(self, mako_idx: int):
        if mako_idx not in [1,2,3,4]:
            print("mako_idx out of the range. Ranges are " + str([1,2,3,4]))
            return
        t,l,b,r = self.chimera_data_command(f"Get-MAKO-Dimension $mako{mako_idx:d}") # top, left, bottom, right
        return map(round, (l,b,r-l+1,t-b+1)) # left, bottom, width, height

    def getMakoFeatureValue(self, mako_idx: int, feature_name: str, feature_type: str):
        if mako_idx not in [1,2,3,4]:
            print("mako_idx out of the range. Ranges are " + str([1,2,3,4]))
            return
        result = self.chimera_data_command(f"Get-MAKO-Feature-Value $mako{mako_idx:d}${feature_name}${feature_type}", data_type=feature_type)
        if feature_type == 'string':
            result = result.tobytes().decode('ascii')
        return result
        
    def setMakoFeatureValue(self, mako_idx: int, feature_name: str, feature_type: str, feature_value: str):
        if mako_idx not in [1,2,3,4]:
            print("mako_idx out of the range. Ranges are " + str([1,2,3,4]))
            return
        return self.chimera_command(f"Set-MAKO-Feature-Value $mako{mako_idx:d}${feature_name}${feature_type}${feature_value}")

    def setPicoScrewPosition(self, pico_idx: int, position: int, update: bool = True):
        if pico_idx not in [1,2,3,4]:
            print("pico_idx out of the range. Ranges are " + str([1,2,3,4]))
            return
        if position>500 or position<-500:
            print("Are you sure you want to move this much???")
            return
        return self.chimera_command(f"Set-PicoScrew-Position ${pico_idx:d}${position:d}${int(update)}")

    def getPicoScrewPositions(self):
        return self.chimera_data_command(f"Get-PicoScrew-Positions", data_type='int')

    def setPicoScrewHomes(self):
        return self.chimera_data_command(f"Set-PicoScrew-Homes", data_type='int')

def experiment_monitoring(exp : ExperimentProcedure, timeout_control = {'use':False, 'timeout':600}):
    # Monitor experiment status
    exp_start_time = time.time()
    timeout = timeout_control.get('timeout', 600)
    time.sleep(1)
    while exp.is_experiment_running():
        elapsed_time = time.time() - exp_start_time
        if timeout_control.get('use', False) and elapsed_time > timeout:
            exp.abort_experiment(save=True)
            print(f"Experiment aborted after {timeout} seconds due to timeout.")
            time.sleep(10)
            break
        print("Waiting for experiment to finish...")
        time.sleep(10) 
    return

def analog_in_calibration_monitoring(exp : ExperimentProcedure, timeout_control = {'use':False, 'timeout':100}):
    # Monitor experiment status
    exp_start_time = time.time()
    timeout = timeout_control.get('timeout', 100)
    time.sleep(1)
    while exp.is_calibration_running():
        elapsed_time = time.time() - exp_start_time
        if timeout_control.get('use', False) and elapsed_time > timeout:
            # exp.abort_experiment(save=True)
            print(f"Calibration aborted after {timeout} seconds due to timeout. THIS SHOULDN'T HAPPEN!!!!")
            time.sleep(10)
            break
        print("Waiting for calibration to finish...")
        time.sleep(1) 
    time.sleep(5) 
    return

def analog_in_calibration(exp : ExperimentProcedure, name : str):
    # Setup calibration details
    exp.run_calibration(name)
    # Monitor calibration status
    analog_in_calibration_monitoring(exp = exp);

def today():
    today = datetime.now()
    # Extract year, month, and day
    year = str(today.year)           # Convert year to string
    month = today.strftime('%B')    # Full month name (already a string)
    day = str(today.day)            # Convert day to string
    return year, month, day


# Example usage:
if __name__ == "__main__":
    # EfieldCalibrationProcedure()
    exp = ExperimentProcedure()
    from RydbergBeamMoveProcedure import extract_beam_position_on_mako
    # exp.run_calibration("prb_pwr")
    # exp.setStaticDDS(580.9,0)
    extract_beam_position_on_mako(exp,4)
    pos = exp.getPicoScrewPositions()
    exp.startMako(3)
    exp.stopMako(3)
    exp.setPicoScrewPosition(1,0)
    exp.setTTL(name="ryd1013trg", value=True)
    exp.setTTL(name="ryd420trg", value=True)
    exp.setDAC()
    exp.setDAC(name='ryd420amp', value=-0.013)
    import matplotlib.pyplot as plt
    t,l,w,h = exp.getMakoImageDimension(4)
    img = exp.getMakoImage(4)
    # exposure = exp.getMakoFeatureValue(4, "ExposureTimeAbs", "double")
    # frame_rate = exp.getMakoFeatureValue(4, "AcquisitionFrameRateAbs", "double")
    # trigger_source = exp.getMakoFeatureValue(4, "TriggerSource", "string")
    exp.setMakoFeatureValue(4, "TriggerSource", "string", "FixedRate")
    plt.pcolormesh(img.reshape(h,w)) # height, width
    plt.gca().set_aspect('equal')
    # plt.plot(img)
    plt.show()

