import os
import glob



def compare_outputs():
    our_path = os.path.join("HW1", "CompArch-hw1-tests", "our_output")
    their_path = os.path.join("HW1", "CompArch-hw1-tests", "ref_results")

    for _, _, files in os.walk(our_path):
        for file_name in files:
            our_file_path = os.path.join(our_path, file_name)
            their_file_path = os.path.join(their_path, file_name)
            print("Comaparing {}".format(file_name))
            cmd = "diff {} {}".format(our_file_path, their_file_path)
            os.system(cmd)
            print("")

def create_outputs():
    in_path = os.path.join("HW1", "CompArch-hw1-tests", "tests")

    for root, _, files in os.walk(in_path):
        for file_name in files:
            file_path = os.path.join(root, file_name)
            file_out_path = os.path.join("HW1", "CompArch-hw1-tests", "our_output", file_name.replace(".trc",".out"))
            cmd = "./HW1/CompArch-hw1/bp_main {} > {}".format(file_path, file_out_path)
            os.system(cmd)


def _main():
    create_outputs()
    compare_outputs()

if __name__ == "__main__":
    _main()