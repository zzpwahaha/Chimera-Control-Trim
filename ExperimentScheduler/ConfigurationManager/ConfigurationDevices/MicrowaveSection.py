import re
from collections import OrderedDict
from ConfigurationManager.ConfigurationSectionClass.ConfigurationSection import ConfigurationSection
from ConfigurationManager.ConfigurationSectionClass.ConfigurationItems import ConfigurationItems

class MicrowaveSection(ConfigurationSection):
    def __init__(self, section_name, data_chunk):
        self.PARAMETER_PATTERN = re.compile(r"/\*(.*?)\*/\s*(.*)")
        self.section_name = section_name
        self.parameters = OrderedDict()
        self.mw_lists = []
        self.parse(data_chunk=data_chunk)

    def parse(self, data_chunk):
        if _match := re.search(r'/\*Control\?\*/ (\d+)', data_chunk):
            self.add_parameter("Control?", _match.group(1))
        else:
            raise ValueError(r"Expecting /*Control?*/ in MicrowaveSection but is not found in .Config")
        num_lists = 0
        if _match := re.search(r'/\*List Size:\*/\s*(\d+)', data_chunk):
            num_lists = int(_match.group(1))
        else:
            raise ValueError(r"Expecting /*List Size:*/ in MicrowaveSection but is not found in .Config")

        # Use regex to capture both the Range delimiter and its content
        split_chunks = re.findall(r'(/\*Freq:\*/.*?)(?=(/\*Freq:\*/|\Z))', data_chunk, re.DOTALL)
        for split_chunk in split_chunks:
            mw_item = ConfigurationItems(split_chunk[0].strip())
            self.mw_lists.append(mw_item)
        assert num_lists==len(self.mw_lists), "len of calibration should match the one in the config."
            

    def print(self):
        """Returns the string representation of the section."""
        strs = [f"{self.section_name}", 
                f"/*Control?*/ {self.parameters['Control?']}",
                f"/*List Size:*/ {len(self.mw_lists)}"]
        for mw_item in self.mw_lists:
            strs.append(mw_item.print())
        strs.append(f"END_{self.section_name}")

        return "\n".join(strs)
    
    def __str__(self):
        return self.print()
    
    def __repr__(self):
        return self.print()

if __name__ == '__main__':
    data_chunk = '/*Control?*/ 1\n/*List Size:*/ 2\n/*Freq:*/ rb87_hfsplitting-50\n/*Power:*/ 15\n/*Freq:*/ mw_scan\n/*Power:*/ 15'
    
    data_chunk = '/*Control?*/ 1\n/*List Size:*/ 2\n/*Freq:*/ rb87_hfsplitting-50\n/*Power:*/ 15\n/*Freq:*/ mw_scan\n/*Power:*/ 15'
    pattern = r'/\*Control\?\*/\s*(\d+)'
    match = re.search(pattern, data_chunk)

    if match:
        number = match.group(1)
        print(number)
    else:
        print("No match found")
    
    variable = MicrowaveSection("MICROWAVE_SYSTEM",data_chunk)
    a = variable.__str__()
    print(variable)