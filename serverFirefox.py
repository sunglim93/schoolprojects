from selenium import webdriver
from subprocess import call, Popen
from time import sleep
import os

def main():
    abspath = os.path.abspath(__file__) # moves to location of script so you don't have to run in folder
    dname = os.path.dirname(abspath)
    os.chdir(dname)
    p = Popen(["python", "-m", "http.server"]) # open the http server
    browser = webdriver.Firefox()
    browser.maximize_window()
    browser.get("http://localhost:8000//tp.html") # change main.html to your filename
    while(1):
        try:
            browser.current_url # tries to see current url of browser, will throw exception if browser is closed
            sleep(1) # save some resources
        except:
            break
    print("Browser closed! Closing server...")
    Popen.terminate(p) # close the http server

if __name__ == '__main__':
    main()
