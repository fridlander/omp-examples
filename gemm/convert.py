import csv
import re
with open('lassen-clang-extralargeUM.csv', 'w', newline='') as f2:
    fieldnames = ['directive', 'objective', 'elapsed_sec']
    writer = csv.DictWriter(f2, fieldnames=fieldnames)
    writer.writeheader()
    with open('results.csv') as csvfile:
        #print("hello")
        reader = csv.DictReader(csvfile)
        stop = False
        for row in reader:
            stop = False
            line = row['p1']
            while not stop:
                #print(line)
                if not re.search(r"#P([0-9]+)", line):
                    stop = True
                for m in re.finditer(r"#P([0-9]+)", line):
                    x = r"#P"+m.group(1) #find the first 
                    y = row["p"+m.group(1)]
                    #print(x, y)
                    line = re.sub(x, y, line)

            #print(line, row['objective'])
            writer.writerow({'directive': line, 'objective': row['objective'], 'elapsed_sec': row['elapsed_sec']})
            
