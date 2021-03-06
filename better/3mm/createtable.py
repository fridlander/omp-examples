"""
This program will create the graph for a given benchmark.
It will sort the csv file lexigraphically and create the graph.
x-axis will have the directives while the y-axis will be the time.
"""
import pandas as pd
from plotnine import *
import os
import matplotlib
import numpy as np

def fun(read):
    cwd = os.getcwd()
    name = cwd.split("/")[5]
    file = pd.read_csv(read)
    baseline = file.loc[file['directive'] == 'baseline']['objective'].values
    filetype = title=read.replace('.csv', '') # that will make lassen-clang-largeUM.csv to lassen-clang-largeUM

    txt = ""
    count_failed = len(file[file['objective'] == 1.7976931348623157e+308])
    file = file[file.objective != 1.7976931348623157e+308] #removes all the times that faild helps a lot when data set is extra large
    if count_failed > 0:
        txt = f"""\n \nThe number of failed runs was: {count_failed}"""

    #need to add a color column, where baseline is red, target teams is blue, parallel for is green.
    file['colour'] = np.full(shape=len(file.index),fill_value='black')

    file.loc[file['directive'].str.contains("pragma omp parallel for",case=False ,na=False), 'colour'] = '#22FF00'
    file.loc[file['directive'].str.contains("target teams distribute",case=False ,na=False), 'colour'] = '#3DC6FF'
    file.loc[file['directive'].str.contains("baseline",case=False ,na=False), 'colour'] = '#E63226'
    
    file['directive'] = file['directive'].astype(str)
    #df[df.columns[0]].count()
    ticks_count = len(set(file[file.columns[0]]))

    print(file)
    print(file.shape[0])

    plot1 = ggplot(file) + theme_dark() + \
                                theme(panel_grid_major = element_blank(), panel_grid_minor = element_blank()) + \
                                geom_point(aes(x='reorder(directive,elapsed_sec)', y='objective', color = 'colour'), size = 3) + \
                                scale_y_continuous(trans = 'log2') + \
                                scale_x_discrete(labels = (range(ticks_count)))+\
                                theme(axis_text_x = element_text(angle = 90, hjust = 1), figure_size=(20, 8)) + \
                                labs(title = filetype +' ' + name + txt, x="Trails", y = "Time") + \
                                theme(legend_title = element_blank()) + \
                                geom_hline(aes(yintercept= float(baseline)), color = '#E63226')

    #(range(file.shape[0]))
    
    #We have other, parallel for, target teams and baseline
    print("black count: " + str(len(file[file['colour'].str.contains('black')])))
    print("target count: " + str(len(file[file['colour'].str.contains('#3DC6FF')])))
    
    if len(file[file['colour'].str.contains('black')]) > 0 and len(file[file['colour'].str.contains('#3DC6FF')]) > 0:
        plot1 = plot1 + scale_color_manual(values =('#22FF00','#3DC6FF','#E63226','black') ,labels=("Parallel For","Target Teams","Baseline", "Other"))

    # We have parallel for, target teams and baseline
    elif len(file[file['colour'].str.contains('#3DC6FF')]) > 0 and len(file[file['colour'].str.contains('black')]) == 0:
        plot1 = plot1 + scale_color_manual(values =('#22FF00','#3DC6FF','#E63226') ,labels=("Parallel For","Target Teams","Baseline"))

    #We have only parallel for and baseline
    elif len(file[file['colour'].str.contains('#3DC6FF')]) == 0 and len(file[file['colour'].str.contains('black')]) == 0:
        plot1 = plot1 + scale_color_manual(values =('#22FF00','#E63226') ,labels=("Parallel For","Baseline"))

    elif len(file[file['colour'].str.contains('#3DC6FF')]) == 0 and len(file[file['colour'].str.contains('black')]) > 0:
        plot1 = plot1 + scale_color_manual(values =('#22FF00','#E63226','black') ,labels=("Parallel For","Baseline", "Other"))
        
    
        
    ggsave(plot1, filetype+name+'.png')
    return 0    

def strip(text):
    try:
        return " ".join(text.split())
    except AttributeError:
        return text

def sort(f):
    
    data = pd.read_table(f, sep=r',',names=['directive', 'objective','elapsed_sec'], converters = {'directive' : strip})
    sorted_df = data.sort_values('objective')[['directive', 'objective','elapsed_sec']]
    sorted_df = sorted_df[sorted_df.objective != 'objective']
    sorted_df.to_csv(f ,index=False)
    """
    data = pd.read_table(f, sep=r',',names=['directive', 'objective','elapsed_sec'], converters = {'directive' : strip})
    data = data[data.objective != 'objective']
    data['elapsed_sec'] = pd.to_numeric(data['elapsed_sec'])
    sorted_df = data.sort_values('elapsed_sec')[['directive', 'objective','elapsed_sec']]
    sorted_df = sorted_df[sorted_df.objective != 'objective']
    sorted_df.to_csv(f ,index=False)
    """

def main():
    print("1)lassen clang large \n2)lassen clang extralarge \n3)lassen xlc large \n4)lassen xlc extralarge \n5)pascal clang large \n6)pascal clang extralarge \n")
    print("7) Run all")
    value = input("Please enter a your choice:\n")
    
    lst = ["lassen-clang-largeUM.csv", "lassen-clang-extralargeUM.csv", "lassen-xl-largeUM.csv", "lassen-xl-extralargeUM.csv", "pascal-clang-largeUM.csv", "pascal-clang-extralargeUM.csv"]

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
            sort(name)
            fun(name)
        return 0
    
    sort(name)
    fun(name)
    return 0

if __name__ == "__main__":
    main()
