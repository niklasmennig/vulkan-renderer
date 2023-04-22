import sys

def handle_include(path):
    global processed_lines
    with open(path) as include_file:
        for line in include_file:
            processed_lines.append(line)

for i in range(1, len(sys.argv)):
    file_path = sys.argv[i]
    filename = file_path.split("/")[-1]
    file_ending = filename.split(".")[-1]
    processed_lines = []


    with open(file_path) as file:
        lines = file.readlines()

        for line in lines:
            if line.startswith("~include"):
                path = line.split("~include")[1].strip().strip("\"")
                handle_include(path)
            else:
                processed_lines.append(line)


    output_path = "shaders/meta/" + filename
    with open(output_path, "w+") as out_file:
        out_file.writelines(processed_lines)
