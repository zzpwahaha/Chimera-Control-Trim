import re

class Variable:
    """Represents a variable in the CONFIG_PARAMETERS section."""
    def __init__(self, data_chunk):
        self.scan_range = None
        self.index = None
        self.name = None
        self.scan_type = None
        self.scan_dimension = None
        self.initial_values = []  # List of initial values
        self.final_values = []  # List of final values
        self.constant_value = None
        self.scope = None
        self.parse(data_chunk=data_chunk)

    def parse(self, data_chunk):
        if _match := re.search(r'/\*Variable #(\d+)\*/', data_chunk):
            self.index = int(_match.group(1))

        # Regex pattern to capture key-value pairs
        pattern = re.compile(r'/\*(.*?):\s*\*/\s*(.*)')
        for match in pattern.finditer(data_chunk):
            key = match.group(1).strip()  # Capture the key and remove extra spaces
            value = match.group(2).strip()  # Capture the value and remove extra spaces

            if key == "Name":
                self.name = value
            elif key == "Scan-Type":
                self.scan_type = value
            elif key == "Scan-Dimension":
                self.scan_dimension = int(value)
            elif key == "Initial Value":
                    self.initial_values.append(value)
            elif key == "Final Value":
                    self.final_values.append(value)
            elif key == "Constant Value":
                self.constant_value = value
            elif key == "Scope":
                self.scope = value
        assert len(self.initial_values)==len(self.final_values), "Initial and Final values must match in count."
        self.scan_range = len(self.initial_values)

    def update(self, name=None, scan_type=None, scan_dimension=None, 
               new_initial_values=None, new_final_values=None, 
               constant_value=None, scope=None):
        """Update the attributes of the variable."""
        if name is not None:
            self.name = name
        if scan_type is not None:
            self.scan_type = scan_type
        if scan_dimension is not None:
            self.scan_dimension = scan_dimension
        if new_initial_values is not None and new_final_values is not None:
            if len(new_initial_values) != len(new_final_values):
                raise ValueError("New initial and final value lists must have the same length.")
            self.initial_values = new_initial_values
            self.final_values = new_final_values
            self.scan_range = len(self.initial_values)  # Update scan_range after values are updated
        if constant_value is not None:
            self.constant_value = constant_value
        if scope is not None:
            self.scope = scope

    def print(self):
        """Returns a string representation of the variable."""
        interleaved_values_str = []
        for initial, final in zip(self.initial_values, self.final_values):
            interleaved_values_str.append(f"/*Initial Value: */\t{initial}")
            interleaved_values_str.append(f"/*Final Value: */\t{final}")
        interleaved_values = "\n".join(interleaved_values_str)
        return (f"/*Variable #{self.index}*/\n"
                f"/*Name:*/\t\t{self.name}\n"
                f"/*Scan-Type:*/\t{self.scan_type}\n"
                f"/*Scan-Dimension:*/\t{self.scan_dimension}\n"
                f"{interleaved_values}\n"
                f"/*Constant Value:*/\t{self.constant_value}\n"
                f"/*Scope:*/\t\t{self.scope}")
    
    def __str__(self):
        return self.print()
    
    def __repr__(self):
        return self.print()


if __name__ == "__main__":
    data_chunk = """/*Variable #0*/
                    /*Name:*/			mot_gradient
                    /*Scan-Type:*/		Constant 
                    /*Scan-Dimension:*/	0
                    /*Initial Value: */	1
                    /*Final Value: */	-1
                    /*Initial Value: */	5
                    /*Final Value: */	-9
                    /*Initial Value: */ 61
                    /*Final Value: */	-2
                    /*Constant Value:*/	-7
                    /*Scope:*/			global"""

    variable = Variable(data_chunk)
    a = variable.__str__()
    print(variable)
