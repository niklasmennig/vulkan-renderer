import sys
import re
import json

current_file = ""

defined_parameters = {}
layout_line_float = "layout(set = 0, binding = 3) readonly buffer FloatParams {float data[];} float_params;"
layout_line_vec3 = "layout(set = 0, binding = 4) readonly buffer Vec3Params {vec4 data[];} vec3_params;"
offsets = {"float": 0, "vec3": 0}
layout_included = False

def full_name(name):
    return current_file + ":" + name

def handle_include(path, line_index):
    global processed_lines
    with open(path) as include_file:
        processed_lines.append("\n")
        for line in include_file:
            processed_lines.append(line)
        processed_lines.append("\n#line " + str(line_index) + "\n")

def handle_parameter_definition(type, name):
    global defined_parameters
    global layout_included
    global processed_lines
    global offsets
    if not full_name(name) in defined_parameters:
        defined_parameters[full_name(name)] = {"type": type, "offset": offsets[type]}
        offsets[type] += 1
    else:
        print("redefinition of {0}", full_name(name))
        quit(1)
    if not layout_included:
        layout_included = True
        processed_lines.append(layout_line_float + "\n")
        processed_lines.append(layout_line_vec3 + "\n")

def handle_parameter_usage(name):
    global processed_lines
    if full_name(name) in defined_parameters:
        type = defined_parameters[full_name(name)]["type"]
        offset = defined_parameters[full_name(name)]["offset"]
        if type == "float":
            return "float_params.data[{0}]".format(offset)
        elif type == "vec3":
            return "vec3_params.data[{0}].xyz".format(offset)
    else:
        print("undefined parameter: {0}".format(full_name(name)))
        return "UNDEFINED"

for i in range(1, len(sys.argv)):
    file_path = sys.argv[i]
    filename = file_path.split("/")[-1]
    file_ending = filename.split(".")[-1]

    current_file = filename
    layout_included = False
    processed_lines = []

    with open(file_path) as file:
        lines = file.readlines()
        line_index = 1

        for line in lines:
            if line.startswith("~include"):
                path = line.split("~include")[1].strip().strip("\"")
                handle_include(path, line_index)
            elif line.startswith("~parameter"):
                split = line.split(" ")
                type = split[1].strip()
                name = split[2].strip()
                handle_parameter_definition(type, name)
            elif "~" in line:
                found = re.findall("~[a-zA-Z0-9_]*", line)
                processed_line = line
                for name in found:
                    replacement = handle_parameter_usage(name[1:])
                    processed_line = processed_line.replace(name, replacement, 1)
                processed_lines.append(processed_line)
            else:
                processed_lines.append(line)
            line_index += 1


    output_path = "shaders/meta/" + filename
    with open(output_path, "w+") as out_file:
        out_file.writelines(processed_lines)

    params_output_path = "shaders/meta/params.json"
    with open(params_output_path, "w+") as params_out_file:
        json.dump(defined_parameters, params_out_file, indent=4)