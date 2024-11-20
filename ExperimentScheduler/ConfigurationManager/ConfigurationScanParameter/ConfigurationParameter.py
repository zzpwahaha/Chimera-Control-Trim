import re
from ConfigurationManager.ConfigurationScanParameter.ScanDimension import ScanDimension
from ConfigurationManager.ConfigurationScanParameter.Variable import Variable

class ConfigurationParameter:
    def __init__(self, data_chunk):
        self.scan_dimensions : list[ScanDimension] = []
        self.variables : list[Variable] = [] 
        self.parse(data_chunk)

    def parse(self, data_chunk):
        # Extract the chunk between RANGE-INFO and /*# Variables: */
        num_dim = 0
        match = re.search(r'RANGE-INFO(.*?)\/\*\# Variables: \*\/', data_chunk, re.DOTALL)
        if match:
            range_info_chunk = match.group(1).strip()
            if _match := re.search(r'/\*\# Scan Dimensions:\*/\s*(\d+)', range_info_chunk):
                num_dim = int(_match.group(1))

            # Parse each dimension separately
            # dim_chunks = re.findall(r'(/\*Dim #\d+:\*/.*?)(?=/\*Dim|\Z)', range_info_chunk, re.DOTALL) # also work
            dim_chunks = re.findall(r'(/\*Dim #\d+:?\*/.*?)(?=/\*Dim #\d+:?|$)', range_info_chunk, re.DOTALL)
            for dim_chunk in dim_chunks:
                dimension = ScanDimension(dim_chunk.strip('\n'))
                self.scan_dimensions.append(dimension)
        assert num_dim==len(self.scan_dimensions), "len of scan_dimension should match the one in the config."

        # Extract the chunk between /*# Variables: */ and the end
        num_var = 0
        match = re.search(r"(/\*# Variations:\s*\*/.*?)\Z", data_chunk, re.DOTALL)
        if match:
            variable_chunk = match.group(1).strip()
            if _match := re.search(r'/\*# Variables:\s*\*/\s*(\d+)', variable_chunk):
                num_var = int(_match.group(1))
            var_chunks = re.findall(r'(/\*Variable #\d+\*/.*?)(?=/\*Variable #\d+\*/|$)', variable_chunk, re.DOTALL)
            for var_chunk in var_chunks:
                variable = Variable(var_chunk.strip('\n'))
                self.variables.append(variable)
        assert num_var==len(self.variables), "len of variables should match the one in the config."

    def print(self):
        scan_dimension_repr = (f"/*# Scan Dimensions:*/\t{len(self.scan_dimensions)}\n"+
                               "\n".join(str(dimension) for dimension in self.scan_dimensions))
        variable_repr = (f"/*# Variables: */\t\t{len(self.variables)}\n"+
                         "\n".join(str(variable) for variable in self.variables))
        return ("CONFIG_PARAMETERS\n"
                "RANGE-INFO\n" + 
                scan_dimension_repr+
                "\n"+
                variable_repr+
                "\nEND_CONFIG_PARAMETERS")

    def __str__(self):
        return self.print()
    
    def __repr__(self):
        return self.print()
    
    def update_scan_dimension(self, index, new_index=None, 
                              range_index=None, new_range=None, new_ranges=None, **kwargs):
        """
        Update a ScanDimension by index or name.
        :param index: The index of the ScanDimension to update.
        :param new_index: (Optional) The new index for the ScanDimension.
        :param range_index: (Optional) The index of the specific range to update.
        :param new_range: (Optional) A ScanRange object to update the specific range at range_index.
        :param new_ranges: (Optional) A list of ScanRange objects to replace the entire ranges list.
        :param kwargs: (Optional) Additional attributes for specific range update.
        :return: None
        """
        scan_dim = self.get_scan_dimension(index)

        if new_index is not None:
            scan_dim.index = new_index

        if new_ranges is not None:
            # Replace the entire list of ranges if new_ranges is provided
            scan_dim.ranges = new_ranges
        elif range_index is not None and new_range is not None:
            # Update a specific range if both range_index and new_range are provided
            if 0 <= range_index < len(scan_dim.ranges):
                scan_dim.ranges[range_index] = new_range
            else:
                raise IndexError("Range index out of range")
        elif range_index is not None and kwargs:
            # Update specific attributes of a range if range_index and kwargs are provided
            if 0 <= range_index < len(scan_dim.ranges):
                scan_dim.update_range(range_index, **kwargs)
            else:
                raise IndexError("Range index out of range")
        elif range_index is not None and new_range is None:
            # If only range_index is provided, raise an error as new_range is expected
            raise ValueError("new_range must be provided if range_index is specified")


        # Revalidate and reorder scan dimensions and variables after update
        self._validate_duplicates()
        self._reorder_variables()
        self._reorder_scan_dimensions()
        self._validate_variables()
        return  # Exit after updating

    def update_variable(self, identifier, **kwargs):
        """
        Update a variable's properties by index or name and ensure it is updated in the self.variables list.
        :param identifier: The index (int) or name (str) of the variable.
        :param new_initial_values: A list of new initial values (optional).
        :param new_final_values: A list of new final values (optional).
        :param kwargs: Other variable properties to update (e.g., name, scan_type).
        :return: None
        """
        # Locate the variable in the list
        for idx, variable in enumerate(self.variables):
            if (isinstance(identifier, int) and variable.index == identifier) or \
            (isinstance(identifier, str) and variable.name == identifier):
                # Update attributes if provided
                variable.update(**kwargs)
                # Replace the variable in the list with the updated one
                self.variables[idx] = variable
                # Revalidate and reorder variables after update
                self._validate_duplicates()
                self._reorder_variables()
                self._reorder_scan_dimensions()
                self._validate_variables()
                return  # Exit after updating

        # If no matching variable is found
        raise ValueError(f"Variable with identifier {identifier} not found.")

    def _reorder_scan_dimensions(self):
        """Reorder the ScanDimensions by their index."""
        for scan_dimension in self.scan_dimensions:
            scan_dimension.sort_ranges()
        self.scan_dimensions.sort(key=lambda dim: dim.index)

    def _reorder_variables(self):
        """Reorder the Variables by their index."""
        self.variables.sort(key=lambda var: var.index)

    def _validate_duplicates(self):
        scan_dim_indices = set()
        variable_indices = set()

        for scan_dimension in self.scan_dimensions:
            if scan_dimension.index in scan_dim_indices:
                raise ValueError(f"Duplicate scan dimension index found: {scan_dimension.index}")
            scan_dim_indices.add(scan_dimension.index)

        for variable in self.variables:
            if variable.index in variable_indices:
                raise ValueError(f"Duplicate variable index found: {variable.index}")
            variable_indices.add(variable.index)

    def _validate_variables(self):
        """Validate that each Variable's scan range matches the corresponding ScanDimension."""
        for variable in self.variables:
            if variable.scan_dimension < len(self.scan_dimensions):
                corresponding_scan_dimension = self.scan_dimensions[variable.scan_dimension]
                if variable.scan_range != len(corresponding_scan_dimension.ranges):
                    print(f"Mismatch detected for Variable #{variable.index}: "
                          f"Variable scan range {variable.scan_range} does not match "
                          f"Scan Dimension {variable.scan_dimension} range length {len(corresponding_scan_dimension.ranges)}.")
                    
                    print(f"Fixing mismatch by updating the variable's scan range to match the Scan Dimension.")
                    # Update the scan_range
                    variable.scan_range = len(corresponding_scan_dimension.ranges)
                    
                    # Ensure the initial and final values lists have the correct length
                    initial_length = len(variable.initial_values)
                    if initial_length < variable.scan_range:
                        variable.initial_values.extend(['0'] * (variable.scan_range - initial_length))
                        variable.final_values.extend(['0'] * (variable.scan_range - initial_length))
                        print(f"Extended initial and final values with zeros to match the new scan range.")
                    elif initial_length > variable.scan_range:
                        variable.initial_values = variable.initial_values[:variable.scan_range]
                        variable.final_values = variable.final_values[:variable.scan_range]
                        print(f"Truncated initial and final values to match the new scan range.")    
                    else:
                        raise IndexError(f"Variable #{variable.index} refers to a non-existent Scan Dimension {variable.scan_dimension}.")

    def get_scan_dimension(self, index):
        """Get a specific ScanDimension by index."""
        for dim in self.scan_dimensions:
            if dim.index == index:
                return dim
        raise ValueError(f"ScanDimension with index {index} not found.")

    def get_variable(self, identifier):
        """
        Retrieve a variable by index or name.
        :param identifier: The index (int) or name (str) of the variable.
        :return: The corresponding Variable object if found, otherwise None.
        """
        if isinstance(identifier, int):
            for variable in self.variables:
                if variable.index == identifier:
                    return variable
        elif isinstance(identifier, str):
            for variable in self.variables:
                if variable.name == identifier:
                    return variable
        return None

if __name__ == "__main__":
    data_chunk=\
"""CONFIG_PARAMETERS
RANGE-INFO
/*# Scan Dimensions:*/	4
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
/*# Variations: */		11
/*Dim #2:*/ 
/*Number of Ranges:*/	1
/*Range #1:*/
/*Left-Inclusive?*/		1
/*Right-Inclusive?*/	1
/*# Variations: */		18
/*Dim #3:*/ 
/*Number of Ranges:*/	1
/*Range #1:*/
/*Left-Inclusive?*/		1
/*Right-Inclusive?*/	1
/*# Variations: */		11
/*Dim #4:*/ 
/*Number of Ranges:*/	1
/*Range #1:*/
/*Left-Inclusive?*/		1
/*Right-Inclusive?*/	1
/*# Variations: */		5
/*# Variables: */ 		3
/*Variable #0*/
/*Name:*/			mot_gradient
/*Scan-Type:*/		Constant 
/*Scan-Dimension:*/	0
/*Initial Value: */	0
/*Final Value: */	0
/*Initial Value: */	0
/*Final Value: */	0
/*Initial Value: */	0
/*Final Value: */	0
/*Constant Value:*/	-7
/*Scope:*/			global

/*Variable #1*/
/*Name:*/			bias_x
/*Scan-Type:*/		Constant 
/*Scan-Dimension:*/	0
/*Initial Value: */	-0.5
/*Final Value: */	1
/*Initial Value: */	0
/*Final Value: */	0
/*Initial Value: */	0
/*Final Value: */	0
/*Constant Value:*/	0.386
/*Scope:*/			global

/*Variable #2*/
/*Name:*/			bias_y
/*Scan-Type:*/		Constant 
/*Scan-Dimension:*/	0
/*Initial Value: */	-1
/*Final Value: */	1
/*Initial Value: */	0
/*Final Value: */	0
/*Initial Value: */	0
/*Final Value: */	0
/*Constant Value:*/	-0.428
/*Scope:*/			global

END_CONFIG_PARAMETERS"""

    variable = ConfigurationParameter(data_chunk)
    a = variable.__str__()
    print(variable)