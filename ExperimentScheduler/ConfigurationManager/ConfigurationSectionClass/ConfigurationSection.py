import re
from collections import OrderedDict
from ConfigurationManager.ConfigurationSectionClass.ConfigurationItems import ConfigurationItems

class ConfigurationSection(ConfigurationItems):
    """Represents a section in the config file."""
    def __init__(self, section_name, data_chunk):
        super().__init__(data_chunk=data_chunk)
        self.section_name = section_name

    def print(self):
        """Returns the string representation of the section."""
        return (f"{self.section_name}\n"
                f"{super().print()}\n"
                f"END_{self.section_name}")
    
    def __str__(self):
        return self.print()
    
    def __repr__(self):
        return self.print()