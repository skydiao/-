CC:=gcc -pthread
TARGET:=op
INCS=-I./ #指定头文件路径
#INCS+= #预留后期增加头文件路径
#LIBS:=-L../library.so -lmy_math #指定库搜索的路径和库名
#LIBS+=
SRCS:=$(wildcard *.c) #搜索当前目录下的所有的.c文件,并将文件名保存到变量SRCS中
OBJS:=$(patsubst %.c,%.o,$(SRCS)) #把SRCS变量中的.c文件替换成同名的.o文件
$(TARGET):$(OBJS) #最终目标 依赖于所有的.o文件
	$(CC) $^ -o $@ $(INCS) $(LIBS) -lm
	cp -rf $(TARGET) /home/china/tftpboot
%.o:%.c
	$(CC) -c $^ -o $@ $(INCS) $(LIBS)
clean:
	rm *.o $(TARGET)
