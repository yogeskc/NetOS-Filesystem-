teststruct *data = malloc(sizeof(teststruct *));
data->x = 10;
data->y = -999;
strcpy(data->name, "blahblah");

LBAwrite((void *)data, 1, 10);

void *data2 = malloc(blockSize);
LBAread(data2, 1, 10);

teststruct *data3 = (teststruct *)data2;
printf("%d %d %s\n", data3->x, data3->y, data3->name);
