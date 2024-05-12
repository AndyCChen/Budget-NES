import sys

file1_path = sys.argv[1]
file2_path = sys.argv[2]

with open(file1_path, "r", encoding="utf-8") as file1:
   file1_num_of_lines = sum(1 for _ in file1)

with open(file2_path, "r", encoding="utf-8") as file2:
   file2_num_of_lines = sum(1 for _ in file2)

if (file1_num_of_lines < file2_num_of_lines):
   num_of_lines = file1_num_of_lines
else:
   num_of_lines = file2_num_of_lines

file1 = open(file1_path, "r", encoding="utf-8")
file2 = open(file2_path, "r", encoding="utf-8")

for line_number in range(1, num_of_lines + 1):
   str1 = file1.readline().strip();
   str2 = file2.readline().strip();
   
   if (str1 != str2):
      print(str1, "  !=  ", str2, "\tLine: ", line_number)
      break;
   else:
      print(str1, "      ", str2, "\tLine: ", line_number)

file1.close()
file2.close()