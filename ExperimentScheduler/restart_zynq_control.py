import time
import subprocess
import os
from ZynqTCPClient import ZynqTCPClient
from SynaccessClient import SynaccessClient
# import TCPClient
# import SSHClient

def ping(target, timeout=5):
    """Ping a target and return whether it is reachable."""
    try:
        result = subprocess.run(
            ['ping', '-n', '1', target],  # Use '-n' for Windows or '-c' for Unix-based systems
            capture_output=True,
            text=True,
            timeout=timeout
        )
        if result.returncode == 0:
            return True, result.stdout
        else:
            return False, result.stderr
    except subprocess.TimeoutExpired:
        return False, "Ping command timed out"
    except Exception as e:
        return False, str(e)

def ping_until_success(target, total_timeout=120, ping_timeout=1, ping_interval=1):
    """Ping a target repeatedly until successful or until total timeout is reached."""
    start_time = time.time()
    while time.time() - start_time < total_timeout:
        success, output = ping(target, timeout=ping_timeout)
        if success:
            return True, output
        print(f"Retrying {target} after {time.time() - start_time:.2f}s")
        time.sleep(ping_interval)
    return False, f"Total timeout of {total_timeout}s reached"

def reboot_zynq(syna_zynq):
    """Reboot the Zynq device."""
    try:
        syna_zynq.reboot(outlet_number=1)
    except Exception as e:
        print(f"Error rebooting Zynq: {e}")

def restart_zynq_control():
    """Restart the Zynq control and associated systems."""
    HOST = '10.10.0.2'
    PORT = 8080

    try:
        tcp_zynq = ZynqTCPClient(HOST, PORT, timeout=1)
        syna_zynq = SynaccessClient("10.10.0.100", 23)
        syna_zynq.connect()

        syna_coil = SynaccessClient("10.10.0.101", 23)
        syna_coil.connect()

        syna_coil.off(outlet_number=1)
        time.sleep(0.5)

        tcp_zynq.send_message("QUIT")
        time.sleep(0.1)
        syna_zynq.reboot(outlet_number=1)

        ping_success, ping_output = ping_until_success(HOST, total_timeout=120)
        if not ping_success:
            print(f"Failed to ping {HOST}. Restarting Zynq control...")
            restart_zynq_control()
            return

        # Path to the script to run
        script_directory = r"C:/Chimera/Chimera-Cryo/ExperimentScheduler"
        script_name = "SSHClient.py"
        script_path = os.path.join(script_directory, script_name)
        command = f'python "{script_path}"'

        # Open a new Command Prompt window and run the script
        cmd_command = f'cmd.exe /c "start cmd.exe /c {command}"'
        # powershell_command = f'powershell.exe -NoExit -Command "{command}"' # somehow only cmd works
        print(f"Running command: {cmd_command}")
        subprocess.Popen(cmd_command, shell=True)

        time.sleep(1)
        syna_coil.on(outlet_number=1)

    except Exception as e:
        print(f"Error in restart_zynq_control: {e}")


if __name__ == "__main__":
    restart_zynq_control()
    # success, result = ping("10.10.0.2")
    # success, output = ping_until_success("10.10.0.2", total_timeout=120)
    # if success:
    #     print(f"Success:\n{output}")
    # else:
    #     print(f"Failed:\n{output}")
