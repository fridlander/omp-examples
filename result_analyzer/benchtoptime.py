"""
Program that will grab the times from each benchmark.
It will first make sure to sort the file by the time.
Change the name of file as needed for compiler,system and data size.
Added a function that will graph a box and whisker plot.
"""
import csv
import os
import shlex
import numpy as np
import pandas as pd
from plotnine import *
import matplotlib

"""
Helps to keep the directive clean without unnecessary spaces.
"""
def strip(text):
    try:
        return " ".join(text.split())
    except AttributeError:
        return text

"""
Goes thorugh each of the directories and grabs the wanted file.
Sorts the file and then makes one big DataFrame.
"""
def fun(name):
    dir = os.getcwd()
    l = []
    time = []
    for subdir, dirs, files in os.walk(dir):
        for filename in files:
            speedup = []
            current = os.path.join(subdir, filename)
            if current.endswith(name) and not subdir.endswith("elapsed_time"):
                print(current)
                data = pd.read_table(current, sep=r',',names=['directive', 'objective','elapsed_sec'], converters = {'directive' : strip})
                sorted_df = data.sort_values('objective')[['directive', 'objective','elapsed_sec']]
                sorted_df = sorted_df[sorted_df.objective != "1.7976931348623157e+308"]
                baseline = sorted_df.loc[sorted_df['directive'] == 'baseline']['objective'].values
                
                sorted_df = sorted_df[sorted_df.objective != 'objective']
                indexremove = sorted_df[sorted_df['directive'] == 'baseline'].index
                sorted_df.drop(indexremove , inplace=True)

                nan = sorted_df[sorted_df['directive'] == ''].index
                sorted_df.at[nan ,'directive'] = '#nan'
                
                count_failed = len(sorted_df[sorted_df['objective'] == "1.7976931348623157e+308"])
                
                #speedup = [for x in time: baseline/x]
                time = sorted_df['objective'].tolist()
                pragma = sorted_df['directive'].tolist()
                
                for x in time:
                    speedup.append(str(float(baseline)/float(x)))
                
                #print(type(time[1]))
                time = ",".join(time)
                pragma = ",".join(pragma)
                speedup = ",".join(speedup)
                bench = os.path.basename((os.path.dirname(current)))
                l.append((bench, pragma, time, count_failed, float(1.0), speedup))
            else:
                continue

    df = pd.DataFrame(l, columns = ['Benchmark', 'Directive','Time', 'Failed', 'baseline', 'speedup'])
    df.to_csv("temp.csv")
    return df


"""
Makes a box and whisker graph for the given DataFrame.

name - the name that contains the system, compiler and size
data - all the data in one csv file
"""
def makegraph(data, name):    
    failed = list(zip(data['Benchmark'],data['Failed']))
    del data['Failed']

    failed = [x for x in failed if x[1] != 0]

    fail_count = "" 
    if(len(failed) > 0):
        fail_count = f"""\n\n Failed runs: {failed}"""
        
    #Have to do the following three lines in order to break the time column that is given as a list
    new_df = pd.DataFrame(data.speedup.str.split(',').tolist(), index=data.Benchmark).stack()
    new_df = new_df.reset_index([0, 'Benchmark'])
    new_df.columns = ['Benchmark', 'speedup']

    new_df['speedup'] = pd.to_numeric(new_df['speedup']) #make it numeric so we will be able to scale it

    #timelst = data.Time.str.split(",")
    timelst = data.speedup.str.split(",")
    dirlst = data.Directive.str.split("#")
    for x in dirlst:
        x.pop(0)

    table = pd.DataFrame({'Benchmark': data.Benchmark ,'Directive': dirlst, 'Time': timelst})

    table = table.apply(pd.Series.explode)

    parallel_for = table.loc[table['Directive'].str.contains("pragma omp parallel for" ,case=False ,na=False)]
    target_teams_parallel = table.loc[table['Directive'].str.contains("target teams" ,case=False ,na=False)]

    print(parallel_for['Directive'].count())
    print(target_teams_parallel['Directive'].count())
    parallel_for['Time'] = pd.to_numeric(parallel_for['Time'])
    target_teams_parallel['Time'] = pd.to_numeric(target_teams_parallel['Time'])
    
    target_teams_parallel = target_teams_parallel.sort_values('Time')
    #target_teams_parallel = target_teams_parallel[target_teams_parallel.Time != -2]

    parallel_for = parallel_for.sort_values('Time')
    #parallel_for = parallel_for[parallel_for.Time != -2]
    
    
    #target_teams_parallel = target_teams_parallel.groupby('Benchmark').first()
    target_teams_parallel = target_teams_parallel.groupby('Benchmark').last()
    target_teams_parallel = target_teams_parallel.reset_index()
    parallel_for = parallel_for.groupby('Benchmark').last()
    parallel_for = parallel_for.reset_index()

    #had to add those colors and then make the color map to each one.
    data['colour'] = np.full(shape=len(data.index),fill_value='red')
    target_teams_parallel['colour'] = np.full(shape=len(target_teams_parallel.index),fill_value='#3DC6FF')
    parallel_for['colour'] = np.full(shape=len(parallel_for.index),fill_value='#22FF00')    

    target_teams_parallel = target_teams_parallel[target_teams_parallel.Time != 1.7976931348623157e+308]
    parallel_for = parallel_for[parallel_for.Time != 1.7976931348623157e+308]

    #trans = 'log2'
    title = name.split(".")[0]
    final = ggplot(new_df) + theme_dark() + geom_boxplot(aes(x='Benchmark', y='speedup')) + theme(figure_size=(20, 10)) + scale_y_continuous(breaks = [0.1,1, 3, 5, 7 ,9]) + ggtitle(title+"\n Speedup Over Baseline" + fail_count)
   
    #final = final + geom_point(data, aes(x='Benchmark', y='baseline', color='colour'), size = 3, show_legend = True)
    
    #geom_hline(aes(yintercept= float(baseline)), color = '#E63226')
    
    #final = final + geom_hline(aes(yintercept= 1, color='red'))
    #adding a point to the graph where the best target teams pargma is
    if(len(target_teams_parallel) !=0):
        final = final + \
                geom_point(target_teams_parallel, aes(x='Benchmark', y='Time', color = "colour"), size = 3,show_legend = True)

    if(len(parallel_for) !=0):
        final = final + \
                geom_point(parallel_for, aes(x='Benchmark', y='Time', color = 'colour'), size = 3,show_legend = True)
        
    
    final = final + scale_color_manual(values = ['#3DC6FF','#22FF00'], labels=("Target Teams","Parallel For"))+theme(legend_title = element_blank())
    #final = final + scale_color_manual(values = ['red','#3DC6FF','#22FF00'], labels=("Baseline","Target Teams","Parallel For"))+theme(legend_title = element_blank())
    final = final + geom_hline(yintercept=float(1), color = 'red')
    ggsave(final, title +'.png')
    return final


"""
Function that grabs the worst 10% of each run

name - the name that contains the system, compiler and size
"""
def grabbad(name):
    dir = os.getcwd()
    l = []
    time = []
    time_list = []
    bench_list = []
    pragma_list = []
    count = 0
    df = pd.DataFrame(columns = ['Benchmark','Pragma','Time'])
    for subdir, dirs, files in os.walk(dir):
        for filename in files:
            current = os.path.join(subdir, filename)
            #print(current)        
            if current.endswith(name) and not subdir.endswith("elapsed_time"):
                #print(current)
                data = pd.read_table(current, index_col=False, header=0 ,sep=r',',names=['directive', 'objective','elapsed_sec'], converters = {'directive' : strip})
                data['objective'] = pd.to_numeric(data['objective'])
                data['directive'] = data['directive'].astype(str)
                data = data[data.objective != 1.7976931348623157e+308]
                sorted_df = data.sort_values('objective')[['directive', 'objective','elapsed_sec']]
                
                #print(sorted_df['objective'])
                sorted_df = data.sort_values('objective')[['directive', 'objective','elapsed_sec']]
                #print(sorted_df)
                time = sorted_df['objective'].tolist()
                dire = sorted_df['directive'].tolist()

                
                """
                worst_time = time[-int((0.1 * len(time))):]
                worst_directive = dire[-int((0.1 * len(dire))):]
                bench = subdir.split("/")[5]
                
                for pragma, time in zip(worst_directive, worst_time):
                    df = df.append({'Benchmark' : bench, 'Pragma' :pragma, 'Time' : time}, 
                                   ignore_index = True)
                """
                
            else:
                continue
    name = name.replace('.csv', '')
    
    #df = pd.DataFrame(bench_list, pragma_list, time_list, columns = ['Benchmark','Pragma','Time'])
    df.to_csv(name + "-bad.csv")
    return df


"""
Helper function to grab the top 10 times from each benchmark.

name - the name that contains the system, compiler and size
"""
def grabtop(name):
    dir = os.getcwd()
    l = []
    time = []
    for subdir, dirs, files in os.walk(dir):
        for filename in files:
            current = os.path.join(subdir, filename)
            #print(current)

            if current.endswith(name) and not subdir.endswith("elapsed_time"):
                print(current)
                data = pd.read_table(current, sep=r',',names=['directive', 'objective','elapsed_sec'], converters = {'directive' : strip})
                sorted_df = data.sort_values('objective')[['directive', 'objective','elapsed_sec']]
                #baseline = sorted_df.loc[sorted_df['directive'] == 'baseline']['objective'].values
                sorted_df = sorted_df[sorted_df.objective != 'objective']
                indexremove = sorted_df[sorted_df['directive'] == 'baseline'].index
                sorted_df.drop(indexremove , inplace=True)

                sorted_df = sorted_df[sorted_df.objective != 'objective']
                sorted_df = sorted_df[sorted_df.objective != '-2']
                sorted_df['objective'] = pd.to_numeric(sorted_df['objective'])
                print(sorted_df)
                time = sorted_df['objective'].tolist()

                bench = subdir.split("/")[5]
                l.append((bench, min(time)))
            else:
                continue

    df = pd.DataFrame(l, columns = ['Benchmark', 'Time'])
    df.to_csv("benchtop.csv")
    return df


def front(self, n):
    return self.iloc[:, :n]

def back(self, n):
    return self.iloc[:, -n:]


"""
Function that will create the speedup box and whisker plot.
The first step will calculating the speedup for every single run and add it as a column to the .csv file
Then graph box and whisker based on the speedup
"""
def speedup_graph():
    return 0
    

def main():
    print("1)lassen clang large \n2)lassen clang extralarge \n3)lassen xlc large \n4)lassen xlc extralarge \n5)pascal clang large \n6)pascal clang extralarge \n")
    print("7) Run all")
    value = input("Please enter a your choice:\n")
    lst = ["lassen-clang-largeUM.csv", "lassen-clang-extralargeUM.csv", "lassen-xl-largeUM.csv", "lassen-xl-extralargeUM.csv", "pascal-clang-largeUM.csv", "pascal-clang-extralargeUM.csv"]

    pd.DataFrame.front = front
    pd.DataFrame.back = back
    if(int(value) == 1):
        name = lst[0]
    elif(int(value) == 2):
        name = lst[1]
    elif(int(value) == 3):
        name = lst[2]
    elif(int(value) == 4):
        name = lst[3]
    elif(int(value) == 5):
        name = lst[4]
    elif(int(value) == 6):
        name = lst[5]
    elif(int(value) == 7):
        for name in lst:
            df = fun(name)
            graph = makegraph(df, name)
            #grabtop(name)

    
    #df = fun(name)
    #graph = makegraph(df, name)
    #$grabtop(name)
    #sort(name)
    #fun(name)
    return 0

if __name__ == "__main__":
    main()
