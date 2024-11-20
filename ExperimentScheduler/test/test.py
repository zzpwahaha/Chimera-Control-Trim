import subprocess

# # Define the command you want to run
# command = 'python C:/Chimera/Chimera-Cryo/ExperimentScheduler/pythonSSH.py'  # Replace with your desired PowerShell command

# # Build the command to open a new PowerShell window and run the specified command
# # -NoExit keeps the window open after the command executes
# powershell_command = f'powershell.exe -NoExit -Command "{command}"'

# # Launch a new PowerShell window
# # subprocess.Popen(['powershell.exe', '-NoExit', '-Command', command])
# subprocess.run('powershell.exe ' + command)
# print('asd')



# # Define the path to the script you want to run
# script_path = 'C:/Chimera/Chimera-Cryo/ExperimentScheduler/pythonSSH.py'  # Replace with the path to your script

# # Start the other script in a new process without blocking the current script
# process = subprocess.Popen(['python', script_path])

# print('asd')


import subprocess
import os

script_directory = r"C:\Chimera\Chimera-Cryo\ExperimentScheduler"
script_name = "test2.py"
script_name = "SSHClient.py"
script_path = os.path.join(script_directory, script_name)
command = f'python "{script_path}"'
cmd_command = f'cmd.exe /c "start cmd.exe /k {command}"'
cmd_command = f'cmd.exe /c "start cmd.exe /k {command}"' # somehow only  cmd works but powershell doesn't
# cmd_command = f'cmd.exe "start cmd.exe /k {command}"'
# cmd_command = f'cmd.exe -NoExit -Command "{command}"'

print(f"Running command: {cmd_command}")

try:
    subprocess.Popen(cmd_command, shell=True)
except Exception as e:
    print(f"An error occurred: {e}")
