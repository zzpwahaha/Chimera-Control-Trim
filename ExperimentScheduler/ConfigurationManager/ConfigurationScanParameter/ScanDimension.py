import re
from ConfigurationManager.ConfigurationScanParameter.ScanRange import ScanRange

class ScanDimension:
    def __init__(self, data_chunk):
        self.index = None
        self.ranges : list[ScanRange] = []
        self.parse(data_chunk)

    def parse(self, data_chunk):
        num_ranges = 0
        # Extract the scan dimensions
        if _match := re.search(r'/\*Dim #(\d+):\*/', data_chunk):
            self.index = int(_match.group(1))-1 # change it back to zero based

        if _match := re.search(r'/\*Number of Ranges:\*/\s*(\d+)', data_chunk):
            num_ranges = int(_match.group(1)) # this is still 1-based, but not used for indexing and only for comparing size

        # Split the data chunk into separate ranges based on the Range delimiter
        # range_chunks = re.split(r'(/\*Range #\d+:\*/)', data_chunk)[1:]  # Skip the first element which is before the first range
        
        # Use regex to capture both the Range delimiter and its content
        range_chunks = re.findall(r'(/\*Range #\d+:\*/.*?)(?=(/\*Range #\d+:|\Z))', data_chunk, re.DOTALL)


        for range_chunk in range_chunks:
            # Create a new Range instance for each range chunk
            range_instance = ScanRange(data_chunk=range_chunk[0].strip())
            self.ranges.append(range_instance)
        assert num_ranges==len(self.ranges), "len of ranges should match the one in the config."

    def print(self):
        self.sort_ranges();
        range_output = [str(range_instance) for range_instance in self.ranges]
        return (f"/*Dim #{self.index+1}:*/\n" 
                f"/*Number of Ranges:*/	{len(self.ranges)}\n"
                + "\n".join(range_output))

    def __str__(self):
        return self.print()
    
    def __repr__(self):
        return self.print()

    def update(self, index=None, ranges=None):
        if index is not None:
            self.index = index
        if ranges is not None:
            self.ranges = ranges

    def update_range(self, range_index, **kwargs):
        if 0 <= range_index < len(self.ranges):
            self.ranges[range_index].update(**kwargs)
        else:
            raise IndexError("Range index out of range")

    def sort_ranges(self):
        self.ranges.sort(key=lambda r: r.index)

if __name__ == "__main__":
    data_chunk = \
    """
    /*Dim #1:*/ 
    /*Number of Ranges:*/	3
    /*Range #1:*/
    /*Left-Inclusive?*/		1
    /*Right-Inclusive?*/	1
    /*# Variations: */		16
    /*Range #2:*/
    /*Left-Inclusive?*/		1
    /*Right-Inclusive?*/	1
    /*# Variations: */		11
    /*Range #3:*/
    /*Left-Inclusive?*/		1
    /*Right-Inclusive?*/	1
    /*# Variations: */		11"""

    variable = ScanDimension(data_chunk)
    a = variable.__str__()
    print(variable)