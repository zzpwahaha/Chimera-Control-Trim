print('asdasd')


import re


# Sample text
text = """Version: 1.2.3
caosjdop
asddg
Version: 50
"""

# Define the pattern
pattern = r'Version:\s*(\S.*?)(?:\s|$)' #re.compile(r'Version:\s*(\S.*?)(?:\s|$)')
pattern = r'Version:\s*(.*?)(?:\s|$)' #re.compile(r'Version:\s*(\S.*?)(?:\s|$)')
# pattern = r'Version:\s*(.*?)\n' #re.compile(r'Version:\s*(\S.*?)(?:\s|$)')
# pattern = r'Version:\s*(.*?)\n' #re.compile(r'Version:\s*(\S.*?)(?:\s|$)')
# pattern = r'Version:\s*(.*?)' #re.compile(r'Version:\s*(\S.*?)(?:\s|$)')
# pattern = r'Version:(.*?)'

# Use re.search to find the match
match = re.search(pattern, text)

if match:
    # Extract the captured group (version)
    version = match.group(1)  # .strip() to remove any surrounding whitespace
    _= match.groups()
    print("Version found:", version)
else:
    print("Version not found.")

# import re

# # Sample text with different scenarios
# text_samples = [
#     "Version: 1.2.3\n",
#     "Version:   ",
#     "Version: 2.3.4",
#     "Version:     Beta-Release",
#     "Version: 3.4.5"
# ]

# # Compile the regex pattern
# pattern = re.compile(r'Version:\s*(\S.*?)(?:\s|$)')

# for text in text_samples:
#     match = pattern.search(text)
#     if match:
#         version = match.group(1).strip()  # Use .strip() to clean up any leading/trailing whitespace
#         print(f"Match found: '{text.strip()}' => Version: '{version}'")
#     else:
#         print(f"No match found for: '{text.strip()}'")


import re

# Sample text
text = "/*Variable #42*/ some code /*Variable #7*/ more code"

# Compile the pattern
pattern = re.compile(r'/\*Variable #(\d+)\*/')

# Use finditer to iterate over all matches
for match in pattern.finditer(text):
    variable_number = match.group(1)
    print("Variable number found:", variable_number)