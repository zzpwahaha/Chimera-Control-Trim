import re
class ScanRange:
    def __init__(self, data_chunk=None, index=None, left_inclusive=None, right_inclusive=None, variations=None):
        if data_chunk is not None:
            # If data_chunk is provided, parse it
            self.parse(data_chunk)
        else:
            # Otherwise, use direct initialization
            self.index : int = index
            self.left_inclusive : bool = left_inclusive
            self.right_inclusive : bool = right_inclusive
            self.variations : int = variations

    def parse(self, data_chunk):
        if _match := re.search(r'/\*Range #(\d+):\*/', data_chunk):
            self.index = int(_match.group(1))-1 # change it back to zero based

        # Regex pattern to capture key-value pairs
        pattern = re.compile(r'/\*(.*?)\s*\*/(?:\t*)(.*)')
        for match in pattern.finditer(data_chunk):
            key = match.group(1).strip()  # Capture the key and remove extra spaces
            value = match.group(2).strip()  # Capture the value and remove extra spaces
            if key == "Left-Inclusive?":
                self.left_inclusive = bool(int(value))
            elif key == "Right-Inclusive?":
                self.right_inclusive = bool(int(value))
            elif key == "# Variations:":
                self.variations = int(value)

    def print(self):
        left_inclusive_str = f"/*Left-Inclusive?*/\t\t{int(self.left_inclusive)}"
        right_inclusive_str = f"/*Right-Inclusive?*/\t{int(self.right_inclusive)}"
        variations_str = f"/*# Variations: */\t\t{self.variations}"
        return (f"/*Range #{self.index+1}:*/\n"
                f"{left_inclusive_str}\n"
                f"{right_inclusive_str}\n"
                f"{variations_str}")
    
    def __str__(self):
        return self.print()
    
    def __repr__(self):
        return self.print()
    
    def update(self, left_inclusive=None, right_inclusive=None, variations=None):
        if left_inclusive is not None:
            self.left_inclusive = left_inclusive
        if right_inclusive is not None:
            self.right_inclusive = right_inclusive
        if variations is not None:
            self.variations = variations


if __name__ == "__main__":
    data_chunk = \
    """/*Range #1:*/
    /*Left-Inclusive?*/     1
    /*Right-Inclusive?*/    1
    /*# Variations: */      16"""

    variable = ScanRange(data_chunk = data_chunk)
    a = variable.__str__()
    print(variable)