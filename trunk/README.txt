OpenNFM

NAND Flash is ubiquitous. You can find them in  iPod, iphone, iPad, MacBook, and of course, all other kinds of non-Apple electronic devices. It provides better performance and reliebility than traditional HDD, and makes your device sustain longer. However, these advantages are not always the case after reviewing some real firmware code of managed-NAND devices(e.g. SD, USB, etc). I realize that my data is in danger! Moreover, a sophisticated NAND FTL and a mature general file system can work together better than most flash file system (that may be the reason why android adopt Ext4 FS over eMMC). The NAND management firmware is so important in both performance and reliability that I developed the Open Nand Flash Manager (OpenNFM, or onfm) for reference. I think it should be useful for people who is using NAND in their design. 

Actually, NAND is not the ideal metarial for mass storage. So, a sophisticated firmware is required to manage NAND and provide capability of read write sectors on NAND just like on normal block devices. The firmware (e.g. OpenNFM) should implement features such as edit out-of-place, static-wear-leveling, power-loss-recovery, and garbage-collect (e.g. reclaim) to solve the limitation of NAND: program erase cycle, sequetial page write, and etc.

In OpenNFM, we implement these features in two layers. One is UBI layer, which manages block resources. It solves the problem of bad-block-recovery and static-wear-leveling. Another is FTL layer. It is a page-mapping FTL, and works based on UBI. FTL is in charge of allocate and reclaim pages. The Hot Data Identifier (HDI) in FTL can separate hot data and cold data, in order to improve the reclaim efficiency. Both UBI and FTL are power-loss-recoverable (PLR). 

To adopt the test-driven-development methodology, we implement a NAND simulator. Then, it is possible to debug and test OpenNFM on desktop PC. You can find the Visual Studio Project in directory: opennfm\prj\sim. With the help of the NAND simulator and unit-test framework, we can improve the quality of OpenNFM code rapidly. After verification on simulated platform, we also ported OpenNFM to LPC3131 (NXP's ARM9 MCU with high speed USB and MLC NAND controller). 

Though we still have many tasks in our TODO list (like sanitize, ncq, ...), OpenNFM is now stable enough to develop managed-NAND devices. It is greatly encouraged to use OpenNFM, and appricated to contribute to OpenNFM. You can find more reference papers and documents in my dropbox [https://www.dropbox.com/s/8spe225jy1hvcpg#view:list].

Here are some proposals to design a high-performance NAND storage system. 

- Page mapping is better than block mapping, though it incurs larger overhead. Besides writing user's data, FTL also need to write some meta-data to NAND for management purpose. Moreover, FTL copies valid data when reclaiming dirty blocks. These extra writing increases the WAF (write amplification factor). With page mapping algorithm, OpenNFM's WAF is as low as 1.13 in most cases. 

- To transfer data between bus (like SD, USB, SATA, ...) and nand flash simultanously, the data should be kept in a dedicated dual-port SRAM. The SRAM can be accessed by bus controller and nand flash controller separatedly, so we can pipeline data between bus and nand flash. 

- When programming ONE page, we should transfer data and then program data. They consume time tT and tP respectively. Because tP is much larger than tT, tP is the bottleneck. In this case, using SLC can achieve better performance than MLC. 

|-tT-|--------tP--------|

- When programming multiple pages acrossing multiple NAND dice, we can interleave page programming on different dice. We can use as many NAND dice as possible. In this case, tT is the bottleneck, instead of tP. The little the tT is, the more dice can be programmed in one tP cycle. So, it is important to verify the timing of NAND data bus. A typical bandwidth of NAND data bus is 40M Byte/s.

|-tT-|--------tP--------|                                DIE 1
     |-tT-|--------tP--------|                           DIE 2
          |-tT-|--------tP--------|                      DIE 3
               |-tT-|--------tP--------|                 DIE 4
                        |-tT-|--------tP--------|        DIE 1

- To design an SSD, a typical SATA bus bandwidth is 300M~600M Byte/s. The tT can be decreased further when adopting ONFI2 or ONFI3 as NAND interface. We also have another choice to increase the transfer speed: multi-channel. Every channel has its own nand flash controller and ECC hardware, so they can send data and program simultanously. The transfer speed from DATA SRAM to channel controller should be very high, at least comparable with (SATA) bus speed (it is not difficult to achieve in a 32-bit bus-width controller). Suppose it consumes tTc to transfer data from DATA SRAM to channel controller, and tT to transfer from channel controller to nand flash. We can again interleave data transfer across multi-channels as follows. There is some calculation overhead for page mapping algorithm, but we can hide it under tTc, because we use DMA to transfer data to channel controller. 

|-tTc-|--------tT--------|                               Channel 1
      |-tTc-|--------tT--------|                         Channel 2
            |-tTc-|--------tT--------|                   Channel 3
                  |-tTc-|--------tT--------|             Channel 4
                         |-tTc-|--------tT--------|      Channel 1


In this multi-channel architecture, the bottleneck lies in both tTc and tBUS (the time to transfer data from SATA bus to DATA SRAM). The dual-port DATA SRAM, as discribed above, can parallelize the transfer time of tTc and tBUS. As a whole, the multi-channel multi-die NAND storage system can make full use of the SATA bus bandwidth. 
