import re
import sys

def calculate_average_time(log):
    time_values = re.findall(r'Time per frame: (\d+\.\d+) ms', log)
    
    if not time_values:
        return None

    # Convert the time values to floats and calculate the average
    time_values_float = [float(value) for value in time_values]
    average_time = sum(time_values_float) / len(time_values_float)

    return average_time

def calculate_average_time_from_file(file_path):
    try:
        with open(file_path, 'r') as file:
            log_content = file.read()
            average_time = calculate_average_time(log_content)
            return average_time
    except FileNotFoundError:
        print(f"Error: File '{file_path}' not found.")
        return None
    except Exception as e:
        print(f"Error: An unexpected error occurred - {e}")
        return None

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python script_name.py log_file.txt")
        sys.exit(1)

    file_path = sys.argv[1]
    average_time = calculate_average_time_from_file(file_path)

    if average_time is not None:
        print(f"Average Time per frame: {average_time:.2f} ms")
    else:
        print("No time values found in the log.")
