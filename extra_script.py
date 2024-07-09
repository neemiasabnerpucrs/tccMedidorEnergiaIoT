import os
Import("env")
def before_upload(source, target, env):
    os.system("taskkill /IM putty.exe /F")

env.AddPreAction("upload", before_upload)