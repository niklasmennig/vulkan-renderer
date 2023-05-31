import sys

def handle_include(path, line_index):
    global processed_lines
    with open(path) as include_file:
        processed_lines.append("\n")
        for line in include_file:
            processed_lines.append(line)
        processed_lines.append("\n#line " + str(line_index) + "\n")

for i in range(1, len(sys.argv)):
    file_path = sys.argv[i]
    filename = file_path.split("/")[-1]
    file_ending = filename.split(".")[-1]
    processed_lines = []


    with open(file_path) as file:
        lines = file.readlines()
        line_index = 1

        for line in lines:
            if line.startswith("~include"):
                path = line.split("~include")[1].strip().strip("\"")
                handle_include(path, line_index)
            else:
                processed_lines.append(line)
            line_index += 1


    output_path = "shaders/meta/" + filename
    with open(output_path, "w+") as out_file:
        out_file.writelines(processed_lines)
