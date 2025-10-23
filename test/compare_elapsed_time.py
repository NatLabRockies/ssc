import pandas as pd
from pathlib import Path
import requests
import sys
import os
import zipfile
import io
import time
from platform import processor

help_info = """
Command Line Options to run this script in order to generate CSVs of time elapsed for tests and to compare them

'python compare_elapsed_time.py gtest_log <path to gtest log file>' 
    Creates the csv in the directory of the log

'python compare_elapsed_time.py compare <path to csv or gtest log> <branch to compare>' 
    Compares the results from a previous run of Github actions by downloading the artifact csv of the given branch

'python compare_elapsed_time.py download_csv <github sha> <output_dir>'
    Downloads the results from workflow for the given SHA and saves it to output dir
"""
access_token = os.getenv("GH_TOKEN")

tested_platforms = ["Windows", "Mac Arm", "Mac Intel", "Linux"]

platform = None
if sys.platform == 'linux':
    platform = "Linux"
elif sys.platform == 'win32':
    platform = "Windows"
elif sys.platform == 'darwin':
    proc = processor()
    if "arm" in proc:
        platform = "Mac Arm"
    else:
        platform = "Mac Intel"
else:
    raise RuntimeError(f"Unrecognized platform {sys.platform}")

def convert_log_to_csv(gtest_log_path):
    test_groups = []
    test_names = []
    test_times = []
    with open(gtest_log_path, 'r') as f:
        for i in range(4):
            next(f)
        for line in f:
            if line[0] != '[':
                continue
            if 'OK' in line or 'FAIL' in line:
                test_arr = line.split('] ')[1].split("(")
                test_name = test_arr[0].strip()
                test_name = test_name.split('.')
                test_group = test_name[0]
                test_name = test_name[-1]
                if len(test_arr) > 1:
                    test_time = float(test_arr[1].split(" ms")[0])
            elif 'test suites' in line:
                test_group = "Total"
                test_name = "Total"
                test_time = float(line.split("(")[1].split(" ms")[0])
            elif 'total' in line:
                test_arr = line.split('from ')[1].split("(")
                test_group = test_arr[0].split()[0]
                test_name = "Total"
                test_time = float(test_arr[1].split(" ms")[0])
            else:
                continue
            test_groups.append(test_group)
            test_names.append(test_name)
            test_times.append(test_time)

    test_df = pd.DataFrame({"Test Group": test_groups, "Test Name": test_names, "Test Times [ms]": test_times})
    out_path = gtest_log_path.parent / "gtest_elapsed_times.csv"
    test_df.to_csv(out_path, index=False)
    print(f"Output csv: {out_path}")
    return test_df


def check_workflow_job_status(workflow_id):
    headers = {
        'Accept': 'application/vnd.github+json',
        'Authorization': f'Bearer {access_token}',
        'X-GitHub-Api-Version': '2022-11-28',
    }

    response = requests.get(f'https://api.github.com/repos/NREL/ssc/actions/runs/{workflow_id}', headers=headers)

    if response.status_code != 200:
        print(response.json())
        raise (f"Error getting data for workflow run {workflow_id}")
    
    conclusion = response.json()['conclusion']

    if conclusion == 'failure':
        return False

    return True

def get_workflow_artifact_branch(base_branch):
    headers = {
        'Accept': 'application/vnd.github+json',
        'Authorization': f'Bearer {access_token}',
        'X-GitHub-Api-Version': '2022-11-28',
    }

    params = {
        "per_page": 100,
        "page": 1
    }

    artifact_to_download = None
    while artifact_to_download is None:
        response = requests.get('https://api.github.com/repos/NREL/ssc/actions/artifacts', headers=headers, params=params)

        if response.status_code != 200:
            print(response.json())
            raise Exception("Failed to Get Workflow Artifacts List")

        artifacts = response.json()['artifacts']

        if len(artifacts) == 0:
            raise Exception(f"Failed find Artifact from branch {base_branch}")

        artifacts = [a for a in artifacts if a['workflow_run']['head_branch'] == base_branch]
        
        artifacts = [a for a in artifacts if (platform in a['name']) and ("Test Time Elapsed" in a['name'])]

        if len(artifacts):
            artifact_to_download = artifacts[0]

        params['page'] += 1

    headers = {
    'Accept': 'application/vnd.github+json',
    'Authorization': f'Bearer {access_token}',
    'X-GitHub-Api-Version': '2022-11-28',
    }

    response = requests.get(artifact_to_download['archive_download_url'], headers=headers)

    z = zipfile.ZipFile(io.BytesIO(response.content)) 
    file_dir = Path(__file__).parent
    z.extractall(file_dir)
    test_df_base = pd.read_csv(file_dir / "gtest_elapsed_times.csv")
    os.remove(file_dir / "gtest_elapsed_times.csv")
    return test_df_base

def retry_request_with_timeout(url, timeout, headers, sha, retry_delay=20 * 60):
    start_time = time.time()
    while time.time() - start_time < timeout:
        response = requests.get(url, headers=headers)

        if response.status_code == 200:
            artifacts = response.json()['artifacts']
            artifacts_sha = [a for a in artifacts if a['workflow_run']['head_sha'] == sha]
            if len(artifacts_sha) > 0:
                artifacts_count = 0
                for platform in tested_platforms:
                    artifacts = [a for a in artifacts_sha if (platform in a['name']) and ("Test Time Elapsed" in a['name'])]
                    if len(artifacts):
                        artifacts_count += 1
                if artifacts_count == len(tested_platforms):
                    return artifacts_sha
        else:
            print(response.json())
            raise requests.exceptions.RequestException
        print(f"Request failed: retrying in {retry_delay} seconds...")
        time.sleep(retry_delay)
    raise TimeoutError(f"Request to {url} timed out after {timeout} seconds")

def get_artifact_from_sha(sha, output_dir=None):
    """
    Give 3 hours for the tests for finish
    """

    headers = {
        'Accept': 'application/vnd.github+json',
        'Authorization': f'Bearer {access_token}',
        'X-GitHub-Api-Version': '2022-11-28',
    }
    try:
        artifacts_sha = retry_request_with_timeout('https://api.github.com/repos/NREL/ssc/actions/artifacts', 60 * 60 * 3, headers, sha)
    except TimeoutError as e:
        print(e)
    except requests.exceptions.RequestException as e:
        print(f"Request failed after multiple retries: {e}")
    
    for platform in tested_platforms:
        artifacts = [a for a in artifacts_sha if (platform in a['name']) and ("Test Time Elapsed" in a['name'])]

        headers = {
        'Accept': 'application/vnd.github+json',
        'Authorization': f'Bearer {access_token}',
        'X-GitHub-Api-Version': '2022-11-28',
        }

        response = requests.get(artifacts[0]['archive_download_url'], headers=headers)

        z = zipfile.ZipFile(io.BytesIO(response.content)) 
        file_dir = Path(__file__).parent
        z.extractall(file_dir)
        test_df_base = pd.read_csv(file_dir / "gtest_elapsed_times.csv")
        os.remove(file_dir / "gtest_elapsed_times.csv")
        if output_dir is not None:
            test_df_base.to_csv(output_dir / f"gtest_elapsed_times_{platform}.csv", index=False)
            print(f"Saved to {str(output_dir / f'gtest_elapsed_times_{platform}.csv')}")
    return

def get_feature_branch():
    workflow_id = os.getenv("WORKFLOW_ID")
    if workflow_id is None:
        raise Exception("Environment variable 'workflow_id' not defined")

    headers = {
    'Accept': 'application/vnd.github+json',
    'Authorization': f'Bearer {access_token}',
    'X-GitHub-Api-Version': '2022-11-28',
    }

    response = requests.get(f'https://api.github.com/repos/NREL/ssc/actions/runs/{workflow_id}', headers=headers)
    if response.status_code != 200:
        print(response.json())
        raise Exception(f"Failed to Get Feature Branch from Workflow ID {workflow_id}")
    
    return response.json()['head_branch']


def compare_df(new_test_df, base_test_df, new_name, base_name, diff_threshold, diff_rel):
    compare_df = new_test_df.merge(base_test_df, how='outer', suffixes=[f" {new_name}", f" {base_name}"], on=['Test Group', 'Test Name'])
    
    feat_col = f"Test Times [ms] {new_name}"
    def_col = f"Test Times [ms] {base_name}"
    compare_df = compare_df[(compare_df[feat_col] > diff_threshold) & (compare_df[def_col] > diff_threshold)] 

    compare_df.loc[:, f"Diff {base_name} [ms]"] = compare_df[feat_col] - compare_df[def_col]
    compare_df = compare_df[compare_df[f"Diff {base_name} [ms]"] != 0]

    if len(compare_df) == 0:
        return True, compare_df

    compare_df.loc[compare_df[def_col] == 0, f"Diff {base_name} [%]"] = 0
    compare_df.loc[compare_df[def_col] != 0, f"Diff {base_name} [%]"] = round((compare_df[feat_col] - compare_df[def_col]) / compare_df[def_col] * 100, 2)

    compare_df = compare_df[compare_df[f"Diff {base_name} [%]"] >= diff_rel * 100]
    if len(compare_df) == 0:
        return True, compare_df
    else:
        return False, compare_df


def compare_time_elapsed(new_test_df, head_test_df, head_branch):
    diff_tol_rel = float(os.getenv("REL_DIFF_TOL"))
    diff_tol_head = float(os.getenv("HEAD_DIFF_TOL"))
    diff_threshold = float(os.getenv("DIFF_THRESHOLD_MS"))
    feature_branch = get_feature_branch()
    if feature_branch == head_branch:
        return True
    
    pass_head, compare_head_df = compare_df(new_test_df, head_test_df, feature_branch, head_branch, diff_threshold, diff_tol_head)

    rel_test_df = pd.read_csv(Path(__file__).parent / "elapsed_time_release" / f"gtest_elapsed_times_{platform}.csv")
    pass_rel, compare_rel_df = compare_df(new_test_df, rel_test_df, feature_branch, "Release", diff_threshold, diff_tol_rel)

    if pass_head and pass_rel:
        return True
    
    comare_df = pd.merge(compare_head_df, compare_rel_df, how='outer')
    comare_df.to_csv(Path(__file__).parent / "failing_test_times.csv", index=False)

    if len(comare_df) > 0:
        return False
    else:
        return True


if __name__ == "__main__":

    if len(sys.argv) == 1:
        raise RuntimeError("Options are 'gtest_log' or 'compare'. Use 'help' to see details")

    if sys.argv[1] == "help":
        print(help_info)
        exit()
    elif sys.argv[1] == "check_workflow":
        if len(sys.argv) < 3:
            raise RuntimeError("Provide workflow id to check")
        workflow_id = sys.argv[2]
        if not check_workflow_job_status(workflow_id):
            raise RuntimeError(f"Workflow {workflow_id} conclusion was not successful.")
    elif sys.argv[1] == "gtest_log":
        if len(sys.argv) < 3:
            raise RuntimeError("Provide path to gtest log file")
        gtest_log_path = Path(sys.argv[2])
        convert_log_to_csv(gtest_log_path)
    elif sys.argv[1] == "compare":
        if len(sys.argv) < 4:
            raise RuntimeError("Provide path to csv or gtest log file, and the name of the branch for comparison")
        
        filename = Path(sys.argv[2])
        base_branch = sys.argv[3]
        head_test_df = get_workflow_artifact_branch(base_branch)
        if Path(filename).suffix != ".csv":
            test_df = convert_log_to_csv(filename)
        else:
            test_df = pd.read_csv(filename)
        if compare_time_elapsed(test_df, head_test_df, head_branch=base_branch):
            print('Pass')
        else:
            print('Fail')
    elif sys.argv[1] == "download_csv":
        if len(sys.argv) < 3:
            raise RuntimeError("Provide the GitHub SHA to download the artifact")
        sha = sys.argv[2]
        output_dir = None
        if len(sys.argv) == 4:
            output_dir = Path(sys.argv[3])
        get_artifact_from_sha(sha, output_dir)
    else:
        raise RuntimeError("Options are 'gtest_log' or 'compare'. Use 'help' to see details")
 
