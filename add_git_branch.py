import subprocess

Import("env")

def get_firmware_specifier_build_flag():    
    #ret = subprocess.run(["git", "describe"], stdout=subprocess.PIPE, text=True) #Uses only annotated tags
    #ret = subprocess.run(["git", "describe", "--tags"], stdout=subprocess.PIPE, text=True) #Uses any tags
    ret = subprocess.run(["git", "rev-parse", "--abbrev-ref", "HEAD"], stdout=subprocess.PIPE, text=True)
    build_version = ret.stdout.strip()
    ret = subprocess.run(["git", "rev-parse", "--short", "HEAD"], stdout=subprocess.PIPE, text=True)
    build_version += " "
    build_version += ret.stdout.strip()
    build_flag = "-DCOMMIT_INFO='\"" + build_version + "\"'"
    #print ("Firmware Revision: " + build_version)
    print ("build_flag = " + build_flag)
    return (build_flag)

env.Append(
    BUILD_FLAGS=[get_firmware_specifier_build_flag()]
)