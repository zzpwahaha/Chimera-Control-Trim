import re
from collections import OrderedDict
from ConfigurationManager.ConfigurationSectionClass.ConfigurationSection import ConfigurationSection
from ConfigurationManager.ConfigurationSectionClass.ConfigurationItems import ConfigurationItems

class ArbGenSection(ConfigurationSection):
    def __init__(self, section_name, data_chunk):
        self.PARAMETER_PATTERN = re.compile(r"/\*(.*?)\*/\s*(.*)")
        self.section_name = section_name
        self.parameters = OrderedDict()
        self.channels = []
        self.channel_nums = []
        self.parse(data_chunk=data_chunk)

    def parse(self, data_chunk):
        if _match := re.search(r'/\*Synced Option:\*/\s*(\d+)', data_chunk):
            self.add_parameter("Synced Option:", _match.group(1))
        else:
            raise ValueError(r"Expecting \*Synced Option:*\ in ArbGenSection but is not found in .Config")

        # Use regex to capture both the Range delimiter and its content
        split_chunks = re.findall(r'(CHANNEL_\d+.*?)(?=(CHANNEL_\d+|\Z))', data_chunk, re.DOTALL)
        for split_chunk in split_chunks:
            # Create a new Range instance for each range chunk
            config_items = ConfigurationItems(split_chunk[0].strip())
            self.channels.append(config_items)
            self.channel_nums.append(int(re.search(r'CHANNEL_(\d+)', split_chunk[0].strip()).group(1)))
        assert max(self.channel_nums)==len(self.channels), "len of channels should match the one in the config."
            

    def print(self):
        """Returns the string representation of the section."""
        strs = [f"{self.section_name}", f"/*Synced Option:*/ {self.parameters['Synced Option:']}"]
        for ch_num, ch in zip(self.channel_nums, self.channels):
            strs.append(f"CHANNEL_{ch_num}")
            strs.append(ch.print())
        strs.append(f"END_{self.section_name}")

        return "\n".join(strs)
    
    def __str__(self):
        return self.print()
    
    def __repr__(self):
        return self.print()

if __name__ == "__main__":
    data_chunk = """SIGLENT_AWG
/*Synced Option:*/ 0
CHANNEL_1
/*Channel Mode:*/				no_control
/*Polarity Invert:*/			0
/*DC Level:*/					"!#EMPTY_STRING#!"
/*DC Calibrated:*/				0
/*Sine Amplitude:*/				0.85
/*Sine Freq:*/					resonance_scan*1000
/*Sine Phase:*/					0
/*Sine Burst:*/					0
/*Sine Calibrated:*/			0
/*Square Amplitude:*/			5
/*Square Freq:*/				2000
/*Square Offset:*/				2.5
/*Square DutyCycle:*/			40
/*Square Phase:*/				pgc_flash_dutycycle
/*Square Burst:*/				1
/*Square Calibrated:*/			0
/*Preloaded Arb Address:*/		"!#EMPTY_STRING#!"
/*Preloaded Arb Calibrated:*/	0
/*Preloaded Arb Burst:*/	0
/*Scripted Arb Address:*/		"!#EMPTY_STRING#!"
/*Scripted Arb Calibrated:*/	0
CHANNEL_2
/*Channel Mode:*/				no_control
/*Polarity Invert:*/			0
/*DC Level:*/					"!#EMPTY_STRING#!"
/*DC Calibrated:*/				0
/*Sine Amplitude:*/				0.85
/*Sine Freq:*/					resonance_scan*1000
/*Sine Phase:*/					phase_scan
/*Sine Burst:*/					0
/*Sine Calibrated:*/			0
/*Square Amplitude:*/			5
/*Square Freq:*/				2000
/*Square Offset:*/				2.5
/*Square DutyCycle:*/			50
/*Square Phase:*/				0
/*Square Burst:*/				1
/*Square Calibrated:*/			0
/*Preloaded Arb Address:*/		"!#EMPTY_STRING#!"
/*Preloaded Arb Calibrated:*/	0
/*Preloaded Arb Burst:*/	0
/*Scripted Arb Address:*/		"!#EMPTY_STRING#!"
/*Scripted Arb Calibrated:*/	0
END_SIGLENT_AWG"""
    variable = ArbGenSection("SIGLENT",data_chunk)
    a = variable.__str__()
    print(variable)

