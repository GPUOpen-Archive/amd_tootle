#!python
import os
import string
import subprocess

# to allow the script to be run from anywhere - not just the cwd - store the absolute path to the script file
scriptRoot = os.path.dirname(os.path.realpath(__file__))

# Update or clone a project folder
def gitclone(gitPath, tagValue, targetRelativePath):
    # convert projectValue to OS specific format
    tmppath = os.path.join(scriptRoot, targetRelativePath)
    # clean up path, collapsing any ../ and converting / to \ for Windows
    targetPath = os.path.normpath(tmppath)
    print("\n\nProcessing project %s"%targetPath)
    if os.path.isdir(targetPath):
        print("\nDirectory " + targetPath + " exists")
    else:
        print("\nDirectory " + targetPath + " does not exist, using 'git clone' to get latest based on tag %s"%tagValue)
        commandArgs = ["git", "clone", gitPath, targetPath]
        p = subprocess.Popen( commandArgs )
        p.wait()
    p = subprocess.Popen(["git", "checkout", "tags/" + tagValue], cwd=targetPath)
    p.wait();
            
if __name__ == "__main__":

    gitclone("https://github.com/Microsoft/DirectXMesh.git", "apr2017", "../../DirectXMesh")
	