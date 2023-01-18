#!/usr/bin/python3

import requests
import json

URL="https://api.github.com/repos"
OWN="hku-ect"
REPO="gazebosc"
MURL="https://api.github.com/repos/hku-ect/gazebosc/commits/master"

VERSION = (0, 4, 0)

def check_github_releases(ver):
    """
    Returns a tuple of the newest version or None
    argument: a tuple of the version to compare with, i.e. ( 1, 1, 23 )
    """        
    req = requests.get(URL + "/" + OWN + "/" + REPO + "/" + "releases")
    if req.status_code == 200:
        """
        github returns a json containing a list of releases. In this
        list we lookup each tag_name entry
        """
        j = req.json()
        for i in j:
            t = i.get("tag_name")
            major, minor, fix = t.split(".")
            if not major.isdigit():     # sometimes the major contains alfa characters
                major = ''.join(ch for ch in major if ch.isdigit())
            if VERSION[0] < int(major):
                print(major, minor, fix)
                break
            elif VERSION[1] < int(minor):
                print(major, minor, fix)
                break
            elif VERSION[2] < int(fix):
                break

        return None

    else:
        print("error retrieving releases overview")
        return None

def check_github_newer_commit(cursha):
    """
    Returns a tuple of latest commits and compare first with cursha
    """        
    req = requests.get(URL + "/" + OWN + "/" + REPO + "/" + "commits/master")
    if req.status_code == 200:
        """
        github returns a json containing a list of commits. In this
        list we lookup the first sha field
        """
        j = req.json()
        t = j.get("sha")
        if not cursha == t[:6]:
            print(cursha, t)
            return True

        return False

    else:
        print("error retrieving latest commit")
        return False

if __name__ == "__main__":
    #check_github_releases(VERSION)
    print(check_github_newer_commit("af3e9"))
