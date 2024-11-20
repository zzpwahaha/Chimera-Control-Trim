import re
from collections import OrderedDict
from ConfigurationManager.Version import Version
from ConfigurationManager.ConfigurationSectionClass.ConfigurationSection import ConfigurationSection
from ConfigurationManager.ConfigurationSectionClass.ConfigurationStaticSection import ConfigurationStaticSection
from ConfigurationManager.ConfigurationScanParameter.ConfigurationParameter import ConfigurationParameter
from ConfigurationManager.ConfigurationDevices.ArbGenSection import ArbGenSection
from ConfigurationManager.ConfigurationDevices.MicrowaveSection import MicrowaveSection
from ConfigurationManager.ConfigurationDevices.CalibrationSection import CalibrationSection

class ConfigurationFile:
    """Represents the entire config file."""
    SECTION_PATTERN = re.compile(r"([\w-]+)(.*?)\n(END_\1)", re.DOTALL)

    def __init__(self, file_path):
        self.file_path = file_path
        self.sections = OrderedDict()
        self.version = None
        self.config_param = None
        self.read_file()

    def reopen(self):
        self.__init__(self.file_path)

    def read_file(self):
        """Reads the content from the config file."""
        with open(self.file_path, 'r') as file:
            content = file.read()
            self.parse_content(content)

    def parse_content(self, content):
        """Parses the content of the config file."""
        self.version = Version(content)
        for match in self.SECTION_PATTERN.finditer(content):
            section_name = match.group(1)
            section_content = match.group(2).strip()
           
            if section_name == "CONFIG_PARAMETERS":
                self.config_param = ConfigurationParameter(section_content)
            elif re.search(".*AWG", section_name):
                self.sections[section_name] = ArbGenSection(section_name, section_content)
            elif re.search("MICROWAVE_SYSTEM", section_name):
                self.sections[section_name] = MicrowaveSection(section_name, section_content)                
            elif (re.search("DATA_ANALYSIS", section_name) or 
                  re.search("ANDOR_PICTURE_MANAGER", section_name)):
                self.sections[section_name] = ConfigurationStaticSection(section_name, section_content)
            elif re.search("CALIBRATION_MANAGER", section_name):
                self.sections[section_name] = CalibrationSection(section_name, section_content)
            else:
                section = ConfigurationSection(section_name, section_content)
                self.sections[section_name] = section

    def get_section(self, section_name):
        """Returns a section by name."""
        return self.sections.get(section_name)

    def add_section(self, section):
        """Adds a new section to the config."""
        self.sections[section.name] = section

    def remove_section(self, section_name):
        """Removes a section by name."""
        if section_name in self.sections:
            del self.sections[section_name]

    def __str__(self):
        """Returns the string representation of the entire config file."""
        version_str = str(self.version)
        section_str = "\n\n".join(str(section) for section in self.sections.values())
        parameter_str = str(self.config_param)
        totol_strs = [version_str, section_str, parameter_str] 
        return "\n".join(totol_strs)

    def sort_sections(self):
        """Sorts the sections alphabetically."""
        self.sections = OrderedDict(sorted(self.sections.items()))

    def modify_parameter(self, section_name, param_name, new_value):
        """Modifies a parameter within a section."""
        if section_name in self.sections:
            self.sections[section_name].parameters[param_name] = new_value

    def get_parameter(self, section_name, param_name):
        """Gets a parameter's value from a section."""
        if section_name in self.sections:
            return self.sections[section_name].parameters.get(param_name)
        return None

    def save(self, output_path=None):
        """Saves the current configuration back to the file."""
        path = output_path if output_path else self.file_path
        with open(path, 'w') as file:
            file.write(str(self))



if __name__ == "__main__":
    # Example usage:
    # config = ConfigManager('C:\\Chimera\\Chimera-Cryo\\Configurations\\ExperimentAutomation\\tweezerloading.Config')
    config = ConfigurationFile('.\\test\\tweezerloading.Config')

    # Get a section
    # siglent_awg_section = config.get_section('SIGLENT_AWG')
    # print(siglent_awg_section)

    # Update a parameter
    # config.modify_parameter('SIGLENT_AWG', 'Sine Amplitude', '0.95')

    config.config_param.update_variable("resonance_scan", 
                                        scan_type="Variable", scan_dimension=None, 
                                        new_initial_values=None, new_final_values=None, 
                                        constant_value=80, scope=None)
    config.config_param.update_scan_dimension(0, new_index=None, 
                                              range_index=1, new_range=None, new_ranges=None,
                                              left_inclusive=None, right_inclusive=None, variations=5)
    
    new_ranges = config.config_param.get_scan_dimension(0).ranges[:2]
    config.config_param.update_scan_dimension(0, new_index=None, 
                                              range_index=None, new_range=None, new_ranges=new_ranges,
                                              left_inclusive=None, right_inclusive=None, variations=None)



    # Save the updated configuration
    config.save('./test/new_config_file.Config')
    config.save('C:/Chimera/Chimera-Cryo/Configurations/CryoTweezerLoading/new_config_file.Config')
