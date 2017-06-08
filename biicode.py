import os
import shutil

bii_dir = os.path.join(os.getcwd(), "tmp_bii")
os.system("bii init tmp_bii")
shutil.copytree(os.path.join(os.getcwd(), "src/google"), "tmp_bii/blocks/google")
os.chdir(bii_dir)
#os.system("bii cpp:build")
os.system("bii publish")
os.chdir("..")
shutil.rmtree(bii_dir)