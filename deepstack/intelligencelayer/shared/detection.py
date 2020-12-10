
import torch
import time
import json
import io
import _thread as thread
from multiprocessing import Process
from PIL import Image,UnidentifiedImageError
import torch.nn.functional as F
import ast
import sqlite3
import numpy as np
import warnings
import sys
import os
sys.path.insert(0, os.path.join(os.path.dirname(os.path.realpath(__file__)), "../../"))

from process import YOLODetector
from shared import SharedOptions

import torchvision.transforms as transforms
import traceback
from PIL import UnidentifiedImageError
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("--model",type=str,default=None)
parser.add_argument("--name",type=str,default=None)

opt = parser.parse_args()

def objectdetection(thread_name: str, delay: float):

    MODE = SharedOptions.MODE
    SHARED_APP_DIR = SharedOptions.SHARED_APP_DIR
    CUDA_MODE = SharedOptions.CUDA_MODE
    db = SharedOptions.db
    TEMP_PATH = SharedOptions.TEMP_PATH

    if opt.name == None:
        IMAGE_QUEUE = "detection_queue"
    else:
        IMAGE_QUEUE = opt.name+"_queue"
    
    if opt.model == None:
        model_path  = os.path.join(SHARED_APP_DIR,SharedOptions.SETTINGS.DETECTION_MODEL)
    else:
        model_path = opt.model
   
    if MODE == "High":

        reso = SharedOptions.SETTINGS.DETECTION_HIGH
        
    elif MODE == "Medium":
        
        reso = SharedOptions.SETTINGS.DETECTION_MEDIUM
        
    elif MODE == "Low":

        reso = SharedOptions.SETTINGS.DETECTION_LOW

    detector = YOLODetector(model_path,reso,cuda=CUDA_MODE)
    while True:
        queue = db.lrange(IMAGE_QUEUE,0,0)

        db.ltrim(IMAGE_QUEUE,len(queue), -1)
        
        if len(queue) > 0:

            for req_data in queue:
                
                req_data = json.JSONDecoder().decode(req_data)


                img_id = req_data["imgid"]
                req_id = req_data["reqid"]
                req_type = req_data["reqtype"]
                threshold = float(req_data["minconfidence"])
               
                try:

                    img = os.path.join(TEMP_PATH,img_id)
            
                    det = detector.predict(img,threshold)
                        
                    outputs = []

                    for *xyxy, conf, cls in reversed(det):
                        x_min = xyxy[0]
                        y_min = xyxy[1]
                        x_max = xyxy[2]
                        y_max = xyxy[3]
                        score = conf.item()

                        label = detector.names[int(cls.item())]

                        detection = {"confidence":score,"label":label, "x_min":int(x_min), "y_min":int(y_min),"x_max":int(x_max), "y_max":int(y_max)}

                        outputs.append(detection)

                    output = {"success":True,"predictions":outputs}

                except UnidentifiedImageError:
                    err_trace = traceback.format_exc()
                    print(err_trace,file=sys.stderr,flush=True)

                    output = {"success":False, "error":"invalid image file","code":400}
                        
                except Exception:

                    err_trace = traceback.format_exc()
                    print(err_trace,file=sys.stderr,flush=True)
 
                    output = {"success":False, "error":"error occured on the server","code":500}
                    
                finally:
                    db.set(req_id,json.dumps(output))
                    if os.path.exists(TEMP_PATH + img_id):
                        os.remove(img)

        time.sleep(delay)

p = Process(target=objectdetection,args=("",SharedOptions.SLEEP_TIME))
p.start()

