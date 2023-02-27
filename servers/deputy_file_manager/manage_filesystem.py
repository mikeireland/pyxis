import os
import time


folder = 

while(1):

    file_ls = sorted(os.listdir(folder))
    if len(file_ls) > 200
        for filename in file_ls[:-5]:
            filename_relPath = os.path.join(folder,filename)
            os.remove(filename_relPath)
    
    time.sleep(10)