![image-20200716101603326](https://raw.githubusercontent.com/zcker/githubPicture/master/20200716101610.png)

在运行中，可以看到函数运行的过程，首先初始化了main函数，其次是sensor模块，然后是网络状态，最后进行初始化网络。

在runLeachSimulation函数中，记录第一次传感器网络剩余的能量为99.9987%，LEACH为运行的轮数，运行1000轮，在两千轮运行后整个传感器网络剩余了10%的能量。而在runLeachSimulation_New中，第一次运行剩余99.987%的能量，运行2000轮，同样剩余10%的能量。

由此可以看出，每轮消耗0.0013%的能量，整个LEACH可以负载2000轮，直接传输协议模拟只能够模拟一轮，但是效率提高了99.95%.