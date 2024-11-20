import re
from collections import OrderedDict

class ConfigurationStaticSection():
    def __init__(self, section_name, data_chunk):
        self.section_name = section_name
        self.data_chunk = data_chunk

    def print(self):
        """Returns the string representation of the section."""
        strs = [f"{self.section_name}", 
                self.data_chunk,
                f"END_{self.section_name}"]
        return "\n".join(strs)
    
    def __str__(self):
        return self.print()
    
    def __repr__(self):
        return self.print()        