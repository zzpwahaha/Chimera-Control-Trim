import re
from collections import OrderedDict

class ConfigurationItems():
    """Represents a section in the config file."""
    def __init__(self, data_chunk):
        self.PARAMETER_PATTERN = re.compile(r"/\*(.*?)\*/\s*(.*)")
        self.parameters = OrderedDict()
        self.parse(data_chunk=data_chunk)

    def parse(self, data_chunk):
        for param_match in self.PARAMETER_PATTERN.finditer(data_chunk):
            param_name, param_value = param_match.groups()
            self.add_parameter(param_name, param_value)

    def add_parameter(self, name, value):
        """Adds a parameter to the section."""
        # if self.parameters.contain
        self.parameters[name] = value

    def get_parameters(self):
        """Returns the parameters in the section."""
        return self.parameters

    def print(self):
        """Returns the string representation of the section."""
        result = []
        for name, value in self.parameters.items():
            result.append(f"/*{name}*/ {value}")
        return "\n".join(result)
    
    def __str__(self):
        return self.print()
    
    def __repr__(self):
        return self.print()