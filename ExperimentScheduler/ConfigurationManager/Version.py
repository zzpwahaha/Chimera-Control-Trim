import re
class Version():
    """Represents a section in the config file specifically for version information."""
    def __init__(self, data_chunk):
        self.version_number = None
        self.parse(data_chunk=data_chunk)

    def parse(self, data_chunk):
        if _match := re.search(r"Version:\s*(.*)", data_chunk):
            self.version_number = _match.group(1)

    def set_version(self, version):
        """Sets the version number for the section."""
        self.version_number = version

    def print(self):
        """Returns the string representation of the version section."""
        return f"Version: {self.version_number}"

    def __str__(self):
        return self.print()
    
    def __repr__(self):
        return self.print()
    

if __name__ == "__main__":
    data_chunk = \
"""Version: 1.0
SCRIPTS
/*Master Script Address:*/ C:/Chimera/Chimera-Cryo/Configurations/CryoTweezerLoading/rydberg_420_1013_excitation.mScript
END_SCRIPTS"""

    variable = Version(data_chunk)
    a = variable.__str__()
    print(variable)