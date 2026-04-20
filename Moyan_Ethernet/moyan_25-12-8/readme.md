# 修改日期25/12/8
 - 修改以下代码
 - 添加定时任务 每五秒进行一次
 ```#if III
		if (oc.learning_mode == 1)
		{	
			if(P_change1 > 25)
			{
				feed_num++;
				// online_cluster_load_from_eeprom(&oc);//保证使用最新数据
				online_cluster_update(&oc, P_total3);
			}
			if(feed_num%33==0)
			{
				online_cluster_save_to_eeprom(&oc);
				SCI_Printf("数量达到i自动保存\n");
			}
		}
		if(oc.learning_mode == 0 && (P_change1>25))
		{
    		ServiceDog();		//喂狗
			//避免浪费空间
			// online_cluster_load_from_eeprom(&oc);
			if(online_cluster_is_abnormal(&oc,P_total1)== -1)
			{	
			SCI_Printf("通道1检测结果异常,距离最近中心：%d 偏移量：%d\n",judge_nearest_idx,judge_nearest_dist);
			LAN_Cluster();  // 发送聚类数据包
			TriggerAlarm();
			}
			else
			SCI_Printf("检测结果为%d类，正常\n",online_cluster_is_abnormal(&oc,P_total1));
			ServiceDog();		//喂狗
		}
#endif
```

## 具体修改内容如下

 为FFT.c文件中的三个函数（`Freq_Analysis1()`、`Freq_Analysis2()`和`Freq_Analysis3()`）添加了5秒定时器功能。修改包括：

1. __添加了5秒定时器逻辑__：使用`TIM0_Cnt_Base`（100ms定时器）作为时间基准，每50次（5秒）执行一次聚类逻辑。

2. __修改了条件判断__：

   - 原来的代码是在`P_change1 > 25`条件下执行聚类逻辑
   - 现在的代码是每5秒执行一次，但仍然检查`P_change1 > 25`条件（对于学习模式）或直接执行异常检测（对于检测模式）

3. __保持了原有功能__：

   - 学习模式：当`P_change1 > 25`时，更新聚类数据
   - 检测模式：每5秒执行一次异常检测
   - 自动保存：当`feed_num%33==0`时自动保存到EEPROM

4. __修复了变量错误__：

   - 将`online_cluster_update(&oc, P_total3)`改为`online_cluster_update(&oc, P_total1)`（对于通道1）
   - 类似地修复了其他通道的变量

5. __添加了喂狗操作__：在检测模式中添加了`ServiceDog()`调用，确保看门狗不会超时。


#修改日期25/12/12

 - 修改以下代码
 - 添加首次运行标志位 `is_first_run`
 - 将 `TIM0_Cnt_Base` 声明改为volatile 确保每次都从内存读取最新值，而不是寄存器缓存
 - 性能优化只调用一次检测函数，用变量存储结果 
 - 原代码在 `learning_mode == 0` 时调用了两次 `online_cluster_is_abnormal`
 - 进入 5 秒逻辑块的最开始和结束都调用 `ServiceDog()`确保安全
 ##修改后代码
 ``` #if III
        // 1. 声明为 volatile，确保编译器每次都从内存读取最新值，而不是使用寄存器缓存
        extern volatile Uint16 TIM0_Cnt_Base; 
        
        // 使用 static 保持状态
        static Uint16 last_cluster_time = 0;
        static Uint16 is_first_run = 1; // 增加首次运行标志位，更加严谨

        // 获取当前时间
        Uint16 current_time = TIM0_Cnt_Base;
        
        // 计算时间差 (利用 Uint16 无符号溢出特性，即使计数器翻转也能正确计算)
        Uint16 time_diff = (Uint16)(current_time - last_cluster_time);

        // 2. 判断时间差是否 >= 50 (50 * 100ms = 5秒)
        // 增加 is_first_run 判断，确保上电后能立刻或者同步后执行
        if(time_diff >= 50 || is_first_run)
        {
            // 更新上次执行时间
            // 技巧：使用 last_cluster_time + 50 而不是 current_time
            // 这样可以避免"时间漂移"，保证严格的每50个tick执行一次
            if(is_first_run) {
                last_cluster_time = current_time;
                is_first_run = 0;
            } else {
                last_cluster_time += 50; 
            }

            // --- 喂狗 (建议：如果处理时间长，在逻辑开始前先喂一次) ---
            ServiceDog(); 

            // --- 聚类逻辑开始 ---
            if (oc.learning_mode == 1)
            {   
                // 学习模式逻辑
                if(P_change1 > 25)
                {
                    feed_num++;
                    // 优化建议：确保 P_total1 变量名正确
                    online_cluster_update(&oc, P_total1); 
                }
                
                // 33次保存一次
                if(feed_num % 33 == 0)
                {
                    online_cluster_save_to_eeprom(&oc);
                    SCI_Printf("数量达到自动保存\n");
                }
            }
            else if(oc.learning_mode == 0) // 使用 else if 结构更清晰
            {
                // 检测模式逻辑
                // 3. 性能优化：只调用一次检测函数，用变量存储结果
                int check_result = online_cluster_is_abnormal(&oc, P_total1);

                if(check_result == -1)
                {   
                    // 异常处理
                    SCI_Printf("通道1检测结果异常, 距离最近中心：%d 偏移量：%d\n", 
                               judge_nearest_idx, judge_nearest_dist);
                    LAN_Cluster();  // 发送数据包
                    TriggerAlarm(); // 触发报警
                }
                else
                {
                    // 正常处理
                    SCI_Printf("检测结果为%d类，正常\n", check_result);
                }
            }
            
            // 逻辑结束再次喂狗，防止上述代码执行时间过长导致看门狗复位
            ServiceDog(); 
        }
#endif
``` 