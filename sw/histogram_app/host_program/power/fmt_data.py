import sys

def format_csv(csv_file, output_file):
    with open(csv_file, 'r') as file:
        lines = file.readlines()

    formatted_lines = []
    for line in lines:
        line = line.strip()
        formatted_line = ""
        count = 0
        for char in line:
            formatted_line += char
            if char == ',':
                count += 1
                if count % 2 == 0:
                    formatted_line += '\n'
        formatted_lines.append(formatted_line)

    with open(output_file, 'w') as file:
        file.write('\n'.join(formatted_lines))


if __name__ == '__main__':
    if len(sys.argv) < 3:
        print("Usage: python script.py input_csv output_csv")
    else:
        input_csv = sys.argv[1]
        output_csv = sys.argv[2]
        format_csv(input_csv, output_csv)
        print("CSV formatted successfully. Formatted data saved in", output_csv)
