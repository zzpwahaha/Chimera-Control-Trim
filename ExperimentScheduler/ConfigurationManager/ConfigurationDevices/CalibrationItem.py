import re
from collections import OrderedDict
from ConfigurationManager.ConfigurationSectionClass.ConfigurationItems import ConfigurationItems

class CalibrationItem(ConfigurationItems):
    def __init__(self, data_chunk):
        self.PARAMETER_PATTERN = re.compile(r"/\*(.*?)\*/\s*(.*)")
        self.recent_cal = None
        self.history_cal = None
        self.parameters = OrderedDict()
        self.add_parameter = super().add_parameter

        self.parse(data_chunk=data_chunk)
        
    def parse(self, data_chunk):
        # Use regex to capture both the Range delimiter and its content
        split_chunks = re.findall(r'(/\*Calibration Name:.*?)/*Recent Calibration Result:', data_chunk, re.DOTALL)
        super().parse(split_chunks[0].strip())
        
        split_chunks = re.findall(r'/\*Recent Calibration Result:\*/(.*?)/\*Historical Calibration Result:', data_chunk, re.DOTALL)
        self.recent_cal = ConfigurationItems(split_chunks[0].strip())

        split_chunks = re.findall(r'/\*Historical Calibration Result:\*/(.*?)\Z', data_chunk, re.DOTALL)
        self.history_cal = ConfigurationItems(split_chunks[0].strip())
            

    def print(self):
        """Returns the string representation of the section."""
        strs = [f"{super().print()}", 
                f"/*Recent Calibration Result:*/",
                self.recent_cal.print(),
                f"/*Historical Calibration Result:*/",
                self.history_cal.print()]
        return "\n".join(strs)
    
    def __str__(self):
        return self.print()
    
    def __repr__(self):
        return self.print()