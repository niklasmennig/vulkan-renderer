import os;
import subprocess;
import glob;

shaders_path = "./shaders"
shaders_meta_path = "./shaders/meta"
shaders_spirv_path = "./shaders/spirv"

shader_compiler_path = "C:/VulkanSDK/1.3.250.0/Bin/glslangValidator.exe"
shader_compiler_args = ["--target-env", "vulkan1.3"]

# remove old compiled shaders
old_files = glob.glob(shaders_spirv_path + "/*")
for f in old_files:
    os.remove(f)

shader_files = [f for f in os.listdir(shaders_path) if os.path.isfile(os.path.join(shaders_path, f)) and (f.endswith("rchit") or f.endswith("rgen") or f.endswith("rmiss") or f.endswith("rcall"))]
shader_paths = ['/'.join([shaders_path, f]) for f in shader_files]

args = ['python', './shader_preprocessor.py']
args.extend(shader_paths)

# run shader preprocessor on shaders (processed shaders written into shaders/meta)
subprocess.run(args)

# run shader compiler (glslangValidator) on preprocessed shaders (compiled spirv written into shaders/spirv)
for file in shader_files:
    ifile = "/".join([shaders_meta_path, file])
    file_name = file.split(".")[0]
    ofile = "/".join([shaders_spirv_path, file_name + ".spv"])
    compile_args = [shader_compiler_path]
    compile_args.extend(shader_compiler_args)
    compile_args.extend(["-l", "-i", ifile, "-o", ofile])
    proc = subprocess.Popen(compile_args, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    output, err = proc.communicate()
    output_lines = output.splitlines()
    for line in output_lines:
        line_string = line.decode()
        if line_string.startswith("ERROR") and "/" in line_string:
            print(line_string)
