package OFMAlgorithm;

import java.io.File;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;

import lpsolve.LpSolveException;

//import lpsolve.LpSolveException;

public class Main {

	public static void main(String[] args){
		double[][] Ratio = new double[20][2];
		double[][] Time = new double[20][2];
		for (int i = 0; i < 20; i++)
		{
			List <Double> SLA = new ArrayList<Double>();
			for (int j = 0; j < 500; j++){
				SLA.add(Math.random()*40+70);
			}

//			{
//				Algorithms test = new Algorithms();
//				
//				VNFinfo ins0 = new VNFinfo();
//				VNFinfo ins1 = new VNFinfo();
//				VNFinfo ins2 = new VNFinfo();
//				VNFinfo ins3 = new VNFinfo();
//				VNFinfo ins4 = new VNFinfo();
//				VNFinfo ins5 = new VNFinfo();
//				VNFinfo ins6 = new VNFinfo();
//				VNFinfo ins7 = new VNFinfo();
//				VNFinfo ins8 = new VNFinfo();
//				VNFinfo ins9 = new VNFinfo();
//
//				ins0.VNFid =0;
//				ins1.VNFid =1;
//				ins2.VNFid =2;
//				ins3.VNFid =3;
//				ins4.VNFid =4;
//				ins5.VNFid =5;
//				ins6.VNFid =6;
//				ins7.VNFid =7;
//				ins8.VNFid =8;
//				ins9.VNFid =9;
//
//				ins0.flow.add(new Flowinfo(128, SLA.get(0)));
//				ins0.flow.add(new Flowinfo(128, SLA.get(1)));
//				ins0.flow.add(new Flowinfo(500, SLA.get(2)));
//				ins0.flow.add(new Flowinfo(114, SLA.get(3)));
//				ins0.flow.add(new Flowinfo(360, SLA.get(4)));
//				ins0.flow.add(new Flowinfo(1838, SLA.get(5)));
//				ins0.flow.add(new Flowinfo(360, SLA.get(6)));
//				ins0.flow.add(new Flowinfo(127, SLA.get(7)));
//				ins0.flow.add(new Flowinfo(91, SLA.get(8)));
//				ins0.flow.add(new Flowinfo(1044, SLA.get(9)));
//				ins0.flow.add(new Flowinfo(240, SLA.get(10)));
//				ins1.flow.add(new Flowinfo(213, SLA.get(11)));
//				ins1.flow.add(new Flowinfo(1700, SLA.get(12)));
//				ins1.flow.add(new Flowinfo(60, SLA.get(13)));
//				ins1.flow.add(new Flowinfo(372, SLA.get(14)));
//				ins2.flow.add(new Flowinfo(91, SLA.get(15)));
//				ins2.flow.add(new Flowinfo(60, SLA.get(16)));
//				ins2.flow.add(new Flowinfo(94, SLA.get(17)));
//				ins2.flow.add(new Flowinfo(3804, SLA.get(18)));
//				ins2.flow.add(new Flowinfo(212, SLA.get(19)));
//				ins2.flow.add(new Flowinfo(3220, SLA.get(20)));
//				ins2.flow.add(new Flowinfo(98, SLA.get(21)));
//				ins2.flow.add(new Flowinfo(60, SLA.get(22)));
//				ins2.flow.add(new Flowinfo(2760, SLA.get(23)));
//				ins3.flow.add(new Flowinfo(17520, SLA.get(24)));
//				ins4.flow.add(new Flowinfo(13081, SLA.get(25)));
//				ins5.flow.add(new Flowinfo(8544, SLA.get(26)));
//				ins5.flow.add(new Flowinfo(1685, SLA.get(27)));
//				ins6.flow.add(new Flowinfo(84, SLA.get(28)));
//				ins6.flow.add(new Flowinfo(282, SLA.get(29)));
//				ins6.flow.add(new Flowinfo(308, SLA.get(30)));
//				ins6.flow.add(new Flowinfo(66, SLA.get(31)));
//				ins6.flow.add(new Flowinfo(60, SLA.get(32)));
//				ins6.flow.add(new Flowinfo(244, SLA.get(33)));
//				ins6.flow.add(new Flowinfo(65, SLA.get(34)));
//				ins6.flow.add(new Flowinfo(60, SLA.get(35)));
//				ins7.flow.add(new Flowinfo(29115, SLA.get(36)));
//				ins8.flow.add(new Flowinfo(98, SLA.get(37)));
//				ins8.flow.add(new Flowinfo(653, SLA.get(38)));
//				ins8.flow.add(new Flowinfo(372, SLA.get(39)));
//				ins8.flow.add(new Flowinfo(720, SLA.get(40)));
//				ins8.flow.add(new Flowinfo(308, SLA.get(41)));
//				ins8.flow.add(new Flowinfo(128, SLA.get(42)));
//				ins8.flow.add(new Flowinfo(116, SLA.get(43)));
//				ins8.flow.add(new Flowinfo(1685, SLA.get(44)));
//				ins9.flow.add(new Flowinfo(29115, SLA.get(45)));
//				ins9.flow.add(new Flowinfo(98, SLA.get(46)));
//				ins9.flow.add(new Flowinfo(130, SLA.get(47)));
//				ins9.flow.add(new Flowinfo(18168, SLA.get(48)));
//				ins9.flow.add(new Flowinfo(60, SLA.get(49)));
//
//				ins0.Refresh();
//				ins1.Refresh();
//				ins2.Refresh();
//				ins3.Refresh();
//				ins4.Refresh();
//				ins5.Refresh();
//				ins6.Refresh();
//				ins7.Refresh();
//				ins8.Refresh();
//				ins9.Refresh();
//
//				test.GlobalVNFinfo.add(ins0);
//				test.GlobalVNFinfo.add(ins1);
//				test.GlobalVNFinfo.add(ins2);
//				test.GlobalVNFinfo.add(ins3);
//				test.GlobalVNFinfo.add(ins4);
//				test.GlobalVNFinfo.add(ins5);
//				test.GlobalVNFinfo.add(ins6);
//				test.GlobalVNFinfo.add(ins7);
//				test.GlobalVNFinfo.add(ins8);
//				test.GlobalVNFinfo.add(ins9);
//				
//				test.VNFNum = 10;
//				long startTime = System.nanoTime();
////				try {
//					Cost[i][0] = test.ScaleInRandom();
//					System.out.println(Cost[i][0]);
////				} catch (LpSolveException e) {
////					e.printStackTrace();
////				}
//				long endTime = System.nanoTime();
//				Time[i][0] = (double) (endTime - startTime)/1000000.0;
////				System.out.println();
//			}
		
			{
				Algorithms test = new Algorithms();
				
				VNFinfo ins0 = new VNFinfo();
				VNFinfo ins1 = new VNFinfo();
				VNFinfo ins2 = new VNFinfo();
				VNFinfo ins3 = new VNFinfo();
				VNFinfo ins4 = new VNFinfo();
				VNFinfo ins5 = new VNFinfo();
				VNFinfo ins6 = new VNFinfo();
				VNFinfo ins7 = new VNFinfo();
				VNFinfo ins8 = new VNFinfo();
				VNFinfo ins9 = new VNFinfo();
				VNFinfo ins10 = new VNFinfo();
				VNFinfo ins11 = new VNFinfo();
				VNFinfo ins12 = new VNFinfo();
				VNFinfo ins13 = new VNFinfo();
				VNFinfo ins14 = new VNFinfo();
				VNFinfo ins15 = new VNFinfo();
				VNFinfo ins16 = new VNFinfo();
				VNFinfo ins17 = new VNFinfo();
				VNFinfo ins18 = new VNFinfo();
				VNFinfo ins19 = new VNFinfo();
//				VNFinfo ins20 = new VNFinfo();
//				VNFinfo ins21 = new VNFinfo();
//				VNFinfo ins22 = new VNFinfo();
//				VNFinfo ins23 = new VNFinfo();
//				VNFinfo ins24 = new VNFinfo();
//				VNFinfo ins25 = new VNFinfo();
//				VNFinfo ins26 = new VNFinfo();
//				VNFinfo ins27 = new VNFinfo();
//				VNFinfo ins28 = new VNFinfo();
//				VNFinfo ins29 = new VNFinfo();
//				VNFinfo ins30 = new VNFinfo();
//				VNFinfo ins31 = new VNFinfo();
//				VNFinfo ins32 = new VNFinfo();
//				VNFinfo ins33 = new VNFinfo();
//				VNFinfo ins34 = new VNFinfo();
//				VNFinfo ins35 = new VNFinfo();
//				VNFinfo ins36 = new VNFinfo();
//				VNFinfo ins37 = new VNFinfo();
//				VNFinfo ins38 = new VNFinfo();
//				VNFinfo ins39 = new VNFinfo();
//				VNFinfo ins40 = new VNFinfo();
//				VNFinfo ins41 = new VNFinfo();
//				VNFinfo ins42 = new VNFinfo();
//				VNFinfo ins43 = new VNFinfo();
//				VNFinfo ins44 = new VNFinfo();
//				VNFinfo ins45 = new VNFinfo();
//				VNFinfo ins46 = new VNFinfo();
//				VNFinfo ins47 = new VNFinfo();
//				VNFinfo ins48 = new VNFinfo();
//				VNFinfo ins49 = new VNFinfo();

				ins0.VNFid =0;
				ins1.VNFid =1;
				ins2.VNFid =2;
				ins3.VNFid =3;
				ins4.VNFid =4;
				ins5.VNFid =5;
				ins6.VNFid =6;
				ins7.VNFid =7;
				ins8.VNFid =8;
				ins9.VNFid =9;
				ins10.VNFid =10;
				ins11.VNFid =11;
				ins12.VNFid =12;
				ins13.VNFid =13;
				ins14.VNFid =14;
				ins15.VNFid =15;
				ins16.VNFid =16;
				ins17.VNFid =17;
				ins18.VNFid =18;
				ins19.VNFid =19;
//				ins20.VNFid =20;
//				ins21.VNFid =21;
//				ins22.VNFid =22;
//				ins23.VNFid =23;
//				ins24.VNFid =24;
//				ins25.VNFid =25;
//				ins26.VNFid =26;
//				ins27.VNFid =27;
//				ins28.VNFid =28;
//				ins29.VNFid =29;
//				ins30.VNFid =30;
//				ins31.VNFid =31;
//				ins32.VNFid =32;
//				ins33.VNFid =33;
//				ins34.VNFid =34;
//				ins35.VNFid =35;
//				ins36.VNFid =36;
//				ins37.VNFid =37;
//				ins38.VNFid =38;
//				ins39.VNFid =39;
//				ins40.VNFid =40;
//				ins41.VNFid =41;
//				ins42.VNFid =42;
//				ins43.VNFid =43;
//				ins44.VNFid =44;
//				ins45.VNFid =45;
//				ins46.VNFid =46;
//				ins47.VNFid =47;
//				ins48.VNFid =48;
//				ins49.VNFid =49;


				ins0.flow.add(new Flowinfo(128, SLA.get(0)));
				ins0.flow.add(new Flowinfo(128, SLA.get(1)));
				ins0.flow.add(new Flowinfo(500, SLA.get(2)));
				ins0.flow.add(new Flowinfo(114, SLA.get(3)));
				ins0.flow.add(new Flowinfo(360, SLA.get(4)));
				ins0.flow.add(new Flowinfo(1838, SLA.get(5)));
				ins0.flow.add(new Flowinfo(360, SLA.get(6)));
				ins0.flow.add(new Flowinfo(127, SLA.get(7)));
				ins0.flow.add(new Flowinfo(91, SLA.get(8)));
				ins0.flow.add(new Flowinfo(1044, SLA.get(9)));
				ins1.flow.add(new Flowinfo(240, SLA.get(10)));
				ins1.flow.add(new Flowinfo(213, SLA.get(11)));
				ins1.flow.add(new Flowinfo(1700, SLA.get(12)));
				ins1.flow.add(new Flowinfo(60, SLA.get(13)));
				ins1.flow.add(new Flowinfo(372, SLA.get(14)));
				ins1.flow.add(new Flowinfo(91, SLA.get(15)));
				ins1.flow.add(new Flowinfo(60, SLA.get(16)));
				ins1.flow.add(new Flowinfo(94, SLA.get(17)));
				ins1.flow.add(new Flowinfo(3804, SLA.get(18)));
				ins1.flow.add(new Flowinfo(212, SLA.get(19)));
				ins2.flow.add(new Flowinfo(3220, SLA.get(20)));
				ins2.flow.add(new Flowinfo(98, SLA.get(21)));
				ins2.flow.add(new Flowinfo(60, SLA.get(22)));
				ins2.flow.add(new Flowinfo(2760, SLA.get(23)));
				ins2.flow.add(new Flowinfo(17520, SLA.get(24)));
				ins2.flow.add(new Flowinfo(13081, SLA.get(25)));
				ins2.flow.add(new Flowinfo(8544, SLA.get(26)));
				ins2.flow.add(new Flowinfo(1685, SLA.get(27)));
				ins2.flow.add(new Flowinfo(84, SLA.get(28)));
				ins2.flow.add(new Flowinfo(282, SLA.get(29)));
				ins3.flow.add(new Flowinfo(308, SLA.get(30)));
				ins3.flow.add(new Flowinfo(66, SLA.get(31)));
				ins3.flow.add(new Flowinfo(60, SLA.get(32)));
				ins3.flow.add(new Flowinfo(244, SLA.get(33)));
				ins3.flow.add(new Flowinfo(65, SLA.get(34)));
				ins3.flow.add(new Flowinfo(60, SLA.get(35)));
				ins3.flow.add(new Flowinfo(29115, SLA.get(36)));
				ins3.flow.add(new Flowinfo(98, SLA.get(37)));
				ins3.flow.add(new Flowinfo(653, SLA.get(38)));
				ins3.flow.add(new Flowinfo(372, SLA.get(39)));
				ins4.flow.add(new Flowinfo(720, SLA.get(40)));
				ins4.flow.add(new Flowinfo(308, SLA.get(41)));
				ins4.flow.add(new Flowinfo(128, SLA.get(42)));
				ins4.flow.add(new Flowinfo(116, SLA.get(43)));
				ins4.flow.add(new Flowinfo(1685, SLA.get(44)));
				ins4.flow.add(new Flowinfo(29115, SLA.get(45)));
				ins4.flow.add(new Flowinfo(98, SLA.get(46)));
				ins4.flow.add(new Flowinfo(130, SLA.get(47)));
				ins4.flow.add(new Flowinfo(18168, SLA.get(48)));
				ins4.flow.add(new Flowinfo(60, SLA.get(49)));
				ins5.flow.add(new Flowinfo(17520, SLA.get(50)));
				ins5.flow.add(new Flowinfo(116, SLA.get(51)));
				ins5.flow.add(new Flowinfo(98, SLA.get(52)));
				ins5.flow.add(new Flowinfo(360, SLA.get(53)));
				ins5.flow.add(new Flowinfo(66, SLA.get(54)));
				ins5.flow.add(new Flowinfo(98, SLA.get(55)));
				ins5.flow.add(new Flowinfo(114, SLA.get(56)));
				ins5.flow.add(new Flowinfo(244, SLA.get(57)));
				ins5.flow.add(new Flowinfo(720, SLA.get(58)));
				ins5.flow.add(new Flowinfo(1685, SLA.get(59)));
				ins6.flow.add(new Flowinfo(372, SLA.get(60)));
				ins6.flow.add(new Flowinfo(60, SLA.get(61)));
				ins6.flow.add(new Flowinfo(1044, SLA.get(62)));
				ins6.flow.add(new Flowinfo(91, SLA.get(63)));
				ins6.flow.add(new Flowinfo(308, SLA.get(64)));
				ins6.flow.add(new Flowinfo(360, SLA.get(65)));
				ins6.flow.add(new Flowinfo(212, SLA.get(66)));
				ins6.flow.add(new Flowinfo(308, SLA.get(67)));
				ins6.flow.add(new Flowinfo(500, SLA.get(68)));
				ins6.flow.add(new Flowinfo(91, SLA.get(69)));
				ins7.flow.add(new Flowinfo(128, SLA.get(70)));
				ins7.flow.add(new Flowinfo(65, SLA.get(71)));
				ins7.flow.add(new Flowinfo(60, SLA.get(72)));
				ins7.flow.add(new Flowinfo(94, SLA.get(73)));
				ins7.flow.add(new Flowinfo(8544, SLA.get(74)));
				ins7.flow.add(new Flowinfo(653, SLA.get(75)));
				ins7.flow.add(new Flowinfo(60, SLA.get(76)));
				ins7.flow.add(new Flowinfo(213, SLA.get(77)));
				ins7.flow.add(new Flowinfo(127, SLA.get(78)));
				ins7.flow.add(new Flowinfo(60, SLA.get(79)));
				ins8.flow.add(new Flowinfo(240, SLA.get(80)));
				ins8.flow.add(new Flowinfo(1685, SLA.get(81)));
				ins8.flow.add(new Flowinfo(130, SLA.get(82)));
				ins8.flow.add(new Flowinfo(13081, SLA.get(83)));
				ins8.flow.add(new Flowinfo(3220, SLA.get(84)));
				ins8.flow.add(new Flowinfo(3804, SLA.get(85)));
				ins8.flow.add(new Flowinfo(282, SLA.get(86)));
				ins8.flow.add(new Flowinfo(2760, SLA.get(87)));
				ins8.flow.add(new Flowinfo(18168, SLA.get(88)));
				ins8.flow.add(new Flowinfo(128, SLA.get(89)));
				ins9.flow.add(new Flowinfo(98, SLA.get(90)));
				ins9.flow.add(new Flowinfo(1700, SLA.get(91)));
				ins9.flow.add(new Flowinfo(60, SLA.get(92)));
				ins9.flow.add(new Flowinfo(1838, SLA.get(93)));
				ins9.flow.add(new Flowinfo(84, SLA.get(94)));
				ins9.flow.add(new Flowinfo(128, SLA.get(95)));
				ins9.flow.add(new Flowinfo(60, SLA.get(96)));
				ins9.flow.add(new Flowinfo(372, SLA.get(97)));
				ins9.flow.add(new Flowinfo(29115, SLA.get(98)));
				ins9.flow.add(new Flowinfo(29115, SLA.get(99)));
				ins10.flow.add(new Flowinfo(116, SLA.get(100)));
				ins10.flow.add(new Flowinfo(372, SLA.get(101)));
				ins10.flow.add(new Flowinfo(60, SLA.get(102)));
				ins10.flow.add(new Flowinfo(91, SLA.get(103)));
				ins10.flow.add(new Flowinfo(308, SLA.get(104)));
				ins10.flow.add(new Flowinfo(2760, SLA.get(105)));
				ins10.flow.add(new Flowinfo(213, SLA.get(106)));
				ins10.flow.add(new Flowinfo(653, SLA.get(107)));
				ins10.flow.add(new Flowinfo(128, SLA.get(108)));
				ins10.flow.add(new Flowinfo(8544, SLA.get(109)));
				ins11.flow.add(new Flowinfo(244, SLA.get(110)));
				ins11.flow.add(new Flowinfo(1044, SLA.get(111)));
				ins11.flow.add(new Flowinfo(60, SLA.get(112)));
				ins11.flow.add(new Flowinfo(360, SLA.get(113)));
				ins11.flow.add(new Flowinfo(60, SLA.get(114)));
				ins11.flow.add(new Flowinfo(1685, SLA.get(115)));
				ins11.flow.add(new Flowinfo(3804, SLA.get(116)));
				ins11.flow.add(new Flowinfo(128, SLA.get(117)));
				ins11.flow.add(new Flowinfo(114, SLA.get(118)));
				ins11.flow.add(new Flowinfo(98, SLA.get(119)));
				ins12.flow.add(new Flowinfo(1700, SLA.get(120)));
				ins12.flow.add(new Flowinfo(308, SLA.get(121)));
				ins12.flow.add(new Flowinfo(500, SLA.get(122)));
				ins12.flow.add(new Flowinfo(29115, SLA.get(123)));
				ins12.flow.add(new Flowinfo(94, SLA.get(124)));
				ins12.flow.add(new Flowinfo(282, SLA.get(125)));
				ins12.flow.add(new Flowinfo(60, SLA.get(126)));
				ins12.flow.add(new Flowinfo(3220, SLA.get(127)));
				ins12.flow.add(new Flowinfo(360, SLA.get(128)));
				ins12.flow.add(new Flowinfo(372, SLA.get(129)));
				ins13.flow.add(new Flowinfo(60, SLA.get(130)));
				ins13.flow.add(new Flowinfo(65, SLA.get(131)));
				ins13.flow.add(new Flowinfo(66, SLA.get(132)));
				ins13.flow.add(new Flowinfo(1685, SLA.get(133)));
				ins13.flow.add(new Flowinfo(130, SLA.get(134)));
				ins13.flow.add(new Flowinfo(1838, SLA.get(135)));
				ins13.flow.add(new Flowinfo(98, SLA.get(136)));
				ins13.flow.add(new Flowinfo(720, SLA.get(137)));
				ins13.flow.add(new Flowinfo(84, SLA.get(138)));
				ins13.flow.add(new Flowinfo(91, SLA.get(139)));
				ins14.flow.add(new Flowinfo(127, SLA.get(140)));
				ins14.flow.add(new Flowinfo(212, SLA.get(141)));
				ins14.flow.add(new Flowinfo(240, SLA.get(142)));
				ins14.flow.add(new Flowinfo(29115, SLA.get(143)));
				ins14.flow.add(new Flowinfo(98, SLA.get(144)));
				ins14.flow.add(new Flowinfo(17520, SLA.get(145)));
				ins14.flow.add(new Flowinfo(128, SLA.get(146)));
				ins14.flow.add(new Flowinfo(13081, SLA.get(147)));
				ins14.flow.add(new Flowinfo(18168, SLA.get(148)));
				ins14.flow.add(new Flowinfo(60, SLA.get(149)));
				ins15.flow.add(new Flowinfo(720, SLA.get(150)));
				ins15.flow.add(new Flowinfo(308, SLA.get(151)));
				ins15.flow.add(new Flowinfo(282, SLA.get(152)));
				ins15.flow.add(new Flowinfo(60, SLA.get(153)));
				ins15.flow.add(new Flowinfo(66, SLA.get(154)));
				ins15.flow.add(new Flowinfo(2760, SLA.get(155)));
				ins15.flow.add(new Flowinfo(500, SLA.get(156)));
				ins15.flow.add(new Flowinfo(13081, SLA.get(157)));
				ins15.flow.add(new Flowinfo(1700, SLA.get(158)));
				ins15.flow.add(new Flowinfo(128, SLA.get(159)));
				ins16.flow.add(new Flowinfo(1685, SLA.get(160)));
				ins16.flow.add(new Flowinfo(308, SLA.get(161)));
				ins16.flow.add(new Flowinfo(84, SLA.get(162)));
				ins16.flow.add(new Flowinfo(60, SLA.get(163)));
				ins16.flow.add(new Flowinfo(127, SLA.get(164)));
				ins16.flow.add(new Flowinfo(244, SLA.get(165)));
				ins16.flow.add(new Flowinfo(94, SLA.get(166)));
				ins16.flow.add(new Flowinfo(360, SLA.get(167)));
				ins16.flow.add(new Flowinfo(372, SLA.get(168)));
				ins16.flow.add(new Flowinfo(91, SLA.get(169)));
				ins17.flow.add(new Flowinfo(60, SLA.get(170)));
				ins17.flow.add(new Flowinfo(60, SLA.get(171)));
				ins17.flow.add(new Flowinfo(65, SLA.get(172)));
				ins17.flow.add(new Flowinfo(1044, SLA.get(173)));
				ins17.flow.add(new Flowinfo(3804, SLA.get(174)));
				ins17.flow.add(new Flowinfo(98, SLA.get(175)));
				ins17.flow.add(new Flowinfo(91, SLA.get(176)));
				ins17.flow.add(new Flowinfo(240, SLA.get(177)));
				ins17.flow.add(new Flowinfo(372, SLA.get(178)));
				ins17.flow.add(new Flowinfo(114, SLA.get(179)));
				ins18.flow.add(new Flowinfo(3220, SLA.get(180)));
				ins18.flow.add(new Flowinfo(60, SLA.get(181)));
				ins18.flow.add(new Flowinfo(98, SLA.get(182)));
				ins18.flow.add(new Flowinfo(213, SLA.get(183)));
				ins18.flow.add(new Flowinfo(653, SLA.get(184)));
				ins18.flow.add(new Flowinfo(128, SLA.get(185)));
				ins18.flow.add(new Flowinfo(130, SLA.get(186)));
				ins18.flow.add(new Flowinfo(1685, SLA.get(187)));
				ins18.flow.add(new Flowinfo(212, SLA.get(188)));
				ins18.flow.add(new Flowinfo(1838, SLA.get(189)));
				ins19.flow.add(new Flowinfo(8544, SLA.get(190)));
				ins19.flow.add(new Flowinfo(360, SLA.get(191)));
				ins19.flow.add(new Flowinfo(128, SLA.get(192)));
				ins19.flow.add(new Flowinfo(116, SLA.get(193)));
				ins19.flow.add(new Flowinfo(29115, SLA.get(194)));
				ins19.flow.add(new Flowinfo(29115, SLA.get(195)));
				ins19.flow.add(new Flowinfo(98, SLA.get(196)));
				ins19.flow.add(new Flowinfo(17520, SLA.get(197)));
				ins19.flow.add(new Flowinfo(60, SLA.get(198)));
				ins19.flow.add(new Flowinfo(18168, SLA.get(199)));
//				ins20.flow.add(new Flowinfo(372, SLA.get(200)));
//				ins20.flow.add(new Flowinfo(60, SLA.get(201)));
//				ins20.flow.add(new Flowinfo(8544, SLA.get(202)));
//				ins20.flow.add(new Flowinfo(1838, SLA.get(203)));
//				ins20.flow.add(new Flowinfo(60, SLA.get(204)));
//				ins20.flow.add(new Flowinfo(13081, SLA.get(205)));
//				ins20.flow.add(new Flowinfo(116, SLA.get(206)));
//				ins20.flow.add(new Flowinfo(98, SLA.get(207)));
//				ins20.flow.add(new Flowinfo(500, SLA.get(208)));
//				ins20.flow.add(new Flowinfo(60, SLA.get(209)));
//				ins21.flow.add(new Flowinfo(3804, SLA.get(210)));
//				ins21.flow.add(new Flowinfo(127, SLA.get(211)));
//				ins21.flow.add(new Flowinfo(66, SLA.get(212)));
//				ins21.flow.add(new Flowinfo(308, SLA.get(213)));
//				ins21.flow.add(new Flowinfo(128, SLA.get(214)));
//				ins21.flow.add(new Flowinfo(653, SLA.get(215)));
//				ins21.flow.add(new Flowinfo(1700, SLA.get(216)));
//				ins21.flow.add(new Flowinfo(94, SLA.get(217)));
//				ins21.flow.add(new Flowinfo(60, SLA.get(218)));
//				ins21.flow.add(new Flowinfo(282, SLA.get(219)));
//				ins22.flow.add(new Flowinfo(1685, SLA.get(220)));
//				ins22.flow.add(new Flowinfo(98, SLA.get(221)));
//				ins22.flow.add(new Flowinfo(308, SLA.get(222)));
//				ins22.flow.add(new Flowinfo(240, SLA.get(223)));
//				ins22.flow.add(new Flowinfo(1044, SLA.get(224)));
//				ins22.flow.add(new Flowinfo(60, SLA.get(225)));
//				ins22.flow.add(new Flowinfo(3220, SLA.get(226)));
//				ins22.flow.add(new Flowinfo(244, SLA.get(227)));
//				ins22.flow.add(new Flowinfo(360, SLA.get(228)));
//				ins22.flow.add(new Flowinfo(360, SLA.get(229)));
//				ins23.flow.add(new Flowinfo(1685, SLA.get(230)));
//				ins23.flow.add(new Flowinfo(720, SLA.get(231)));
//				ins23.flow.add(new Flowinfo(130, SLA.get(232)));
//				ins23.flow.add(new Flowinfo(91, SLA.get(233)));
//				ins23.flow.add(new Flowinfo(212, SLA.get(234)));
//				ins23.flow.add(new Flowinfo(114, SLA.get(235)));
//				ins23.flow.add(new Flowinfo(29115, SLA.get(236)));
//				ins23.flow.add(new Flowinfo(128, SLA.get(237)));
//				ins23.flow.add(new Flowinfo(91, SLA.get(238)));
//				ins23.flow.add(new Flowinfo(29115, SLA.get(239)));
//				ins24.flow.add(new Flowinfo(60, SLA.get(240)));
//				ins24.flow.add(new Flowinfo(65, SLA.get(241)));
//				ins24.flow.add(new Flowinfo(98, SLA.get(242)));
//				ins24.flow.add(new Flowinfo(2760, SLA.get(243)));
//				ins24.flow.add(new Flowinfo(17520, SLA.get(244)));
//				ins24.flow.add(new Flowinfo(84, SLA.get(245)));
//				ins24.flow.add(new Flowinfo(213, SLA.get(246)));
//				ins24.flow.add(new Flowinfo(128, SLA.get(247)));
//				ins24.flow.add(new Flowinfo(372, SLA.get(248)));
//				ins24.flow.add(new Flowinfo(18168, SLA.get(249)));
//				ins25.flow.add(new Flowinfo(372, SLA.get(250)));
//				ins25.flow.add(new Flowinfo(60, SLA.get(251)));
//				ins25.flow.add(new Flowinfo(8544, SLA.get(252)));
//				ins25.flow.add(new Flowinfo(1838, SLA.get(253)));
//				ins25.flow.add(new Flowinfo(60, SLA.get(254)));
//				ins25.flow.add(new Flowinfo(13081, SLA.get(255)));
//				ins25.flow.add(new Flowinfo(116, SLA.get(256)));
//				ins25.flow.add(new Flowinfo(98, SLA.get(257)));
//				ins25.flow.add(new Flowinfo(500, SLA.get(258)));
//				ins25.flow.add(new Flowinfo(60, SLA.get(259)));
//				ins26.flow.add(new Flowinfo(3804, SLA.get(260)));
//				ins26.flow.add(new Flowinfo(127, SLA.get(261)));
//				ins26.flow.add(new Flowinfo(66, SLA.get(262)));
//				ins26.flow.add(new Flowinfo(308, SLA.get(263)));
//				ins26.flow.add(new Flowinfo(128, SLA.get(264)));
//				ins26.flow.add(new Flowinfo(653, SLA.get(265)));
//				ins26.flow.add(new Flowinfo(1700, SLA.get(266)));
//				ins26.flow.add(new Flowinfo(94, SLA.get(267)));
//				ins26.flow.add(new Flowinfo(60, SLA.get(268)));
//				ins26.flow.add(new Flowinfo(282, SLA.get(269)));
//				ins27.flow.add(new Flowinfo(1685, SLA.get(270)));
//				ins27.flow.add(new Flowinfo(98, SLA.get(271)));
//				ins27.flow.add(new Flowinfo(308, SLA.get(272)));
//				ins27.flow.add(new Flowinfo(240, SLA.get(273)));
//				ins27.flow.add(new Flowinfo(1044, SLA.get(274)));
//				ins27.flow.add(new Flowinfo(60, SLA.get(275)));
//				ins27.flow.add(new Flowinfo(3220, SLA.get(276)));
//				ins27.flow.add(new Flowinfo(244, SLA.get(277)));
//				ins27.flow.add(new Flowinfo(360, SLA.get(278)));
//				ins27.flow.add(new Flowinfo(360, SLA.get(279)));
//				ins28.flow.add(new Flowinfo(1685, SLA.get(280)));
//				ins28.flow.add(new Flowinfo(720, SLA.get(281)));
//				ins28.flow.add(new Flowinfo(130, SLA.get(282)));
//				ins28.flow.add(new Flowinfo(91, SLA.get(283)));
//				ins28.flow.add(new Flowinfo(212, SLA.get(284)));
//				ins28.flow.add(new Flowinfo(114, SLA.get(285)));
//				ins28.flow.add(new Flowinfo(29115, SLA.get(286)));
//				ins28.flow.add(new Flowinfo(128, SLA.get(287)));
//				ins28.flow.add(new Flowinfo(91, SLA.get(288)));
//				ins28.flow.add(new Flowinfo(29115, SLA.get(289)));
//				ins29.flow.add(new Flowinfo(60, SLA.get(290)));
//				ins29.flow.add(new Flowinfo(65, SLA.get(291)));
//				ins29.flow.add(new Flowinfo(98, SLA.get(292)));
//				ins29.flow.add(new Flowinfo(2760, SLA.get(293)));
//				ins29.flow.add(new Flowinfo(17520, SLA.get(294)));
//				ins29.flow.add(new Flowinfo(84, SLA.get(295)));
//				ins29.flow.add(new Flowinfo(213, SLA.get(296)));
//				ins29.flow.add(new Flowinfo(128, SLA.get(297)));
//				ins29.flow.add(new Flowinfo(372, SLA.get(298)));
//				ins29.flow.add(new Flowinfo(18168, SLA.get(299)));
//				ins30.flow.add(new Flowinfo(372, SLA.get(300)));
//				ins30.flow.add(new Flowinfo(60, SLA.get(301)));
//				ins30.flow.add(new Flowinfo(8544, SLA.get(302)));
//				ins30.flow.add(new Flowinfo(1838, SLA.get(303)));
//				ins30.flow.add(new Flowinfo(60, SLA.get(304)));
//				ins30.flow.add(new Flowinfo(13081, SLA.get(305)));
//				ins30.flow.add(new Flowinfo(116, SLA.get(306)));
//				ins30.flow.add(new Flowinfo(98, SLA.get(307)));
//				ins30.flow.add(new Flowinfo(500, SLA.get(308)));
//				ins30.flow.add(new Flowinfo(60, SLA.get(309)));
//				ins31.flow.add(new Flowinfo(3804, SLA.get(310)));
//				ins31.flow.add(new Flowinfo(127, SLA.get(311)));
//				ins31.flow.add(new Flowinfo(66, SLA.get(312)));
//				ins31.flow.add(new Flowinfo(308, SLA.get(313)));
//				ins31.flow.add(new Flowinfo(128, SLA.get(314)));
//				ins31.flow.add(new Flowinfo(653, SLA.get(315)));
//				ins31.flow.add(new Flowinfo(1700, SLA.get(316)));
//				ins31.flow.add(new Flowinfo(94, SLA.get(317)));
//				ins31.flow.add(new Flowinfo(60, SLA.get(318)));
//				ins31.flow.add(new Flowinfo(282, SLA.get(319)));
//				ins32.flow.add(new Flowinfo(1685, SLA.get(320)));
//				ins32.flow.add(new Flowinfo(98, SLA.get(321)));
//				ins32.flow.add(new Flowinfo(308, SLA.get(322)));
//				ins32.flow.add(new Flowinfo(240, SLA.get(323)));
//				ins32.flow.add(new Flowinfo(1044, SLA.get(324)));
//				ins32.flow.add(new Flowinfo(60, SLA.get(325)));
//				ins32.flow.add(new Flowinfo(3220, SLA.get(326)));
//				ins32.flow.add(new Flowinfo(244, SLA.get(327)));
//				ins32.flow.add(new Flowinfo(360, SLA.get(328)));
//				ins32.flow.add(new Flowinfo(360, SLA.get(329)));
//				ins33.flow.add(new Flowinfo(1685, SLA.get(330)));
//				ins33.flow.add(new Flowinfo(720, SLA.get(331)));
//				ins33.flow.add(new Flowinfo(130, SLA.get(332)));
//				ins33.flow.add(new Flowinfo(91, SLA.get(333)));
//				ins33.flow.add(new Flowinfo(212, SLA.get(334)));
//				ins33.flow.add(new Flowinfo(114, SLA.get(335)));
//				ins33.flow.add(new Flowinfo(29115, SLA.get(336)));
//				ins33.flow.add(new Flowinfo(128, SLA.get(337)));
//				ins33.flow.add(new Flowinfo(91, SLA.get(338)));
//				ins33.flow.add(new Flowinfo(29115, SLA.get(339)));
//				ins34.flow.add(new Flowinfo(60, SLA.get(340)));
//				ins34.flow.add(new Flowinfo(65, SLA.get(341)));
//				ins34.flow.add(new Flowinfo(98, SLA.get(342)));
//				ins34.flow.add(new Flowinfo(2760, SLA.get(343)));
//				ins34.flow.add(new Flowinfo(17520, SLA.get(344)));
//				ins34.flow.add(new Flowinfo(84, SLA.get(345)));
//				ins34.flow.add(new Flowinfo(213, SLA.get(346)));
//				ins34.flow.add(new Flowinfo(128, SLA.get(347)));
//				ins34.flow.add(new Flowinfo(372, SLA.get(348)));
//				ins34.flow.add(new Flowinfo(18168, SLA.get(349)));
//				ins35.flow.add(new Flowinfo(91, SLA.get(350)));
//				ins35.flow.add(new Flowinfo(244, SLA.get(351)));
//				ins35.flow.add(new Flowinfo(65, SLA.get(352)));
//				ins35.flow.add(new Flowinfo(13081, SLA.get(353)));
//				ins35.flow.add(new Flowinfo(128, SLA.get(354)));
//				ins35.flow.add(new Flowinfo(8544, SLA.get(355)));
//				ins35.flow.add(new Flowinfo(91, SLA.get(356)));
//				ins35.flow.add(new Flowinfo(1685, SLA.get(357)));
//				ins35.flow.add(new Flowinfo(98, SLA.get(358)));
//				ins35.flow.add(new Flowinfo(60, SLA.get(359)));
//				ins36.flow.add(new Flowinfo(84, SLA.get(360)));
//				ins36.flow.add(new Flowinfo(2760, SLA.get(361)));
//				ins36.flow.add(new Flowinfo(60, SLA.get(362)));
//				ins36.flow.add(new Flowinfo(282, SLA.get(363)));
//				ins36.flow.add(new Flowinfo(372, SLA.get(364)));
//				ins36.flow.add(new Flowinfo(3804, SLA.get(365)));
//				ins36.flow.add(new Flowinfo(1838, SLA.get(366)));
//				ins36.flow.add(new Flowinfo(130, SLA.get(367)));
//				ins36.flow.add(new Flowinfo(60, SLA.get(368)));
//				ins36.flow.add(new Flowinfo(98, SLA.get(369)));
//				ins37.flow.add(new Flowinfo(213, SLA.get(370)));
//				ins37.flow.add(new Flowinfo(360, SLA.get(371)));
//				ins37.flow.add(new Flowinfo(308, SLA.get(372)));
//				ins37.flow.add(new Flowinfo(116, SLA.get(373)));
//				ins37.flow.add(new Flowinfo(212, SLA.get(374)));
//				ins37.flow.add(new Flowinfo(114, SLA.get(375)));
//				ins37.flow.add(new Flowinfo(720, SLA.get(376)));
//				ins37.flow.add(new Flowinfo(653, SLA.get(377)));
//				ins37.flow.add(new Flowinfo(66, SLA.get(378)));
//				ins37.flow.add(new Flowinfo(60, SLA.get(379)));
//				ins38.flow.add(new Flowinfo(240, SLA.get(380)));
//				ins38.flow.add(new Flowinfo(29115, SLA.get(381)));
//				ins38.flow.add(new Flowinfo(372, SLA.get(382)));
//				ins38.flow.add(new Flowinfo(98, SLA.get(383)));
//				ins38.flow.add(new Flowinfo(1044, SLA.get(384)));
//				ins38.flow.add(new Flowinfo(29115, SLA.get(385)));
//				ins38.flow.add(new Flowinfo(308, SLA.get(386)));
//				ins38.flow.add(new Flowinfo(60, SLA.get(387)));
//				ins38.flow.add(new Flowinfo(17520, SLA.get(388)));
//				ins38.flow.add(new Flowinfo(128, SLA.get(389)));
//				ins39.flow.add(new Flowinfo(94, SLA.get(390)));
//				ins39.flow.add(new Flowinfo(500, SLA.get(391)));
//				ins39.flow.add(new Flowinfo(128, SLA.get(392)));
//				ins39.flow.add(new Flowinfo(3220, SLA.get(393)));
//				ins39.flow.add(new Flowinfo(1700, SLA.get(394)));
//				ins39.flow.add(new Flowinfo(1685, SLA.get(395)));
//				ins39.flow.add(new Flowinfo(360, SLA.get(396)));
//				ins39.flow.add(new Flowinfo(60, SLA.get(397)));
//				ins39.flow.add(new Flowinfo(18168, SLA.get(398)));
//				ins39.flow.add(new Flowinfo(127, SLA.get(399)));
//				ins40.flow.add(new Flowinfo(3220, SLA.get(400)));
//				ins40.flow.add(new Flowinfo(128, SLA.get(401)));
//				ins40.flow.add(new Flowinfo(60, SLA.get(402)));
//				ins40.flow.add(new Flowinfo(127, SLA.get(403)));
//				ins40.flow.add(new Flowinfo(60, SLA.get(404)));
//				ins40.flow.add(new Flowinfo(240, SLA.get(405)));
//				ins40.flow.add(new Flowinfo(98, SLA.get(406)));
//				ins40.flow.add(new Flowinfo(720, SLA.get(407)));
//				ins40.flow.add(new Flowinfo(500, SLA.get(408)));
//				ins40.flow.add(new Flowinfo(98, SLA.get(409)));
//				ins41.flow.add(new Flowinfo(1838, SLA.get(410)));
//				ins41.flow.add(new Flowinfo(128, SLA.get(411)));
//				ins41.flow.add(new Flowinfo(60, SLA.get(412)));
//				ins41.flow.add(new Flowinfo(91, SLA.get(413)));
//				ins41.flow.add(new Flowinfo(98, SLA.get(414)));
//				ins41.flow.add(new Flowinfo(360, SLA.get(415)));
//				ins41.flow.add(new Flowinfo(91, SLA.get(416)));
//				ins41.flow.add(new Flowinfo(60, SLA.get(417)));
//				ins41.flow.add(new Flowinfo(653, SLA.get(418)));
//				ins41.flow.add(new Flowinfo(13081, SLA.get(419)));
//				ins42.flow.add(new Flowinfo(212, SLA.get(420)));
//				ins42.flow.add(new Flowinfo(1044, SLA.get(421)));
//				ins42.flow.add(new Flowinfo(308, SLA.get(422)));
//				ins42.flow.add(new Flowinfo(282, SLA.get(423)));
//				ins42.flow.add(new Flowinfo(84, SLA.get(424)));
//				ins42.flow.add(new Flowinfo(360, SLA.get(425)));
//				ins42.flow.add(new Flowinfo(128, SLA.get(426)));
//				ins42.flow.add(new Flowinfo(116, SLA.get(427)));
//				ins42.flow.add(new Flowinfo(8544, SLA.get(428)));
//				ins42.flow.add(new Flowinfo(308, SLA.get(429)));
//				ins43.flow.add(new Flowinfo(29115, SLA.get(430)));
//				ins43.flow.add(new Flowinfo(1685, SLA.get(431)));
//				ins43.flow.add(new Flowinfo(94, SLA.get(432)));
//				ins43.flow.add(new Flowinfo(372, SLA.get(433)));
//				ins43.flow.add(new Flowinfo(1700, SLA.get(434)));
//				ins43.flow.add(new Flowinfo(66, SLA.get(435)));
//				ins43.flow.add(new Flowinfo(60, SLA.get(436)));
//				ins43.flow.add(new Flowinfo(65, SLA.get(437)));
//				ins43.flow.add(new Flowinfo(213, SLA.get(438)));
//				ins43.flow.add(new Flowinfo(130, SLA.get(439)));
//				ins44.flow.add(new Flowinfo(60, SLA.get(440)));
//				ins44.flow.add(new Flowinfo(114, SLA.get(441)));
//				ins44.flow.add(new Flowinfo(372, SLA.get(442)));
//				ins44.flow.add(new Flowinfo(1685, SLA.get(443)));
//				ins44.flow.add(new Flowinfo(244, SLA.get(444)));
//				ins44.flow.add(new Flowinfo(29115, SLA.get(445)));
//				ins44.flow.add(new Flowinfo(3804, SLA.get(446)));
//				ins44.flow.add(new Flowinfo(2760, SLA.get(447)));
//				ins44.flow.add(new Flowinfo(17520, SLA.get(448)));
//				ins44.flow.add(new Flowinfo(18168, SLA.get(449)));
//				ins45.flow.add(new Flowinfo(128, SLA.get(450)));
//				ins45.flow.add(new Flowinfo(360, SLA.get(451)));
//				ins45.flow.add(new Flowinfo(60, SLA.get(452)));
//				ins45.flow.add(new Flowinfo(94, SLA.get(453)));
//				ins45.flow.add(new Flowinfo(84, SLA.get(454)));
//				ins45.flow.add(new Flowinfo(282, SLA.get(455)));
//				ins45.flow.add(new Flowinfo(720, SLA.get(456)));
//				ins45.flow.add(new Flowinfo(65, SLA.get(457)));
//				ins45.flow.add(new Flowinfo(653, SLA.get(458)));
//				ins45.flow.add(new Flowinfo(1700, SLA.get(459)));
//				ins46.flow.add(new Flowinfo(372, SLA.get(460)));
//				ins46.flow.add(new Flowinfo(128, SLA.get(461)));
//				ins46.flow.add(new Flowinfo(3220, SLA.get(462)));
//				ins46.flow.add(new Flowinfo(1044, SLA.get(463)));
//				ins46.flow.add(new Flowinfo(308, SLA.get(464)));
//				ins46.flow.add(new Flowinfo(98, SLA.get(465)));
//				ins46.flow.add(new Flowinfo(3804, SLA.get(466)));
//				ins46.flow.add(new Flowinfo(98, SLA.get(467)));
//				ins46.flow.add(new Flowinfo(308, SLA.get(468)));
//				ins46.flow.add(new Flowinfo(91, SLA.get(469)));
//				ins47.flow.add(new Flowinfo(1685, SLA.get(470)));
//				ins47.flow.add(new Flowinfo(60, SLA.get(471)));
//				ins47.flow.add(new Flowinfo(2760, SLA.get(472)));
//				ins47.flow.add(new Flowinfo(60, SLA.get(473)));
//				ins47.flow.add(new Flowinfo(8544, SLA.get(474)));
//				ins47.flow.add(new Flowinfo(360, SLA.get(475)));
//				ins47.flow.add(new Flowinfo(1685, SLA.get(476)));
//				ins47.flow.add(new Flowinfo(60, SLA.get(477)));
//				ins47.flow.add(new Flowinfo(29115, SLA.get(478)));
//				ins47.flow.add(new Flowinfo(500, SLA.get(479)));
//				ins48.flow.add(new Flowinfo(60, SLA.get(480)));
//				ins48.flow.add(new Flowinfo(18168, SLA.get(481)));
//				ins48.flow.add(new Flowinfo(114, SLA.get(482)));
//				ins48.flow.add(new Flowinfo(66, SLA.get(483)));
//				ins48.flow.add(new Flowinfo(240, SLA.get(484)));
//				ins48.flow.add(new Flowinfo(29115, SLA.get(485)));
//				ins48.flow.add(new Flowinfo(98, SLA.get(486)));
//				ins48.flow.add(new Flowinfo(244, SLA.get(487)));
//				ins48.flow.add(new Flowinfo(91, SLA.get(488)));
//				ins48.flow.add(new Flowinfo(17520, SLA.get(489)));
//				ins49.flow.add(new Flowinfo(128, SLA.get(490)));
//				ins49.flow.add(new Flowinfo(213, SLA.get(491)));
//				ins49.flow.add(new Flowinfo(372, SLA.get(492)));
//				ins49.flow.add(new Flowinfo(13081, SLA.get(493)));
//				ins49.flow.add(new Flowinfo(116, SLA.get(494)));
//				ins49.flow.add(new Flowinfo(130, SLA.get(495)));
//				ins49.flow.add(new Flowinfo(1838, SLA.get(496)));
//				ins49.flow.add(new Flowinfo(212, SLA.get(497)));
//				ins49.flow.add(new Flowinfo(60, SLA.get(498)));
//				ins49.flow.add(new Flowinfo(127, SLA.get(499)));

				ins0.Refresh();
				ins1.Refresh();
				ins2.Refresh();
				ins3.Refresh();
				ins4.Refresh();
				ins5.Refresh();
				ins6.Refresh();
				ins7.Refresh();
				ins8.Refresh();
				ins9.Refresh();
				ins10.Refresh();
				ins11.Refresh();
				ins12.Refresh();
				ins13.Refresh();
				ins14.Refresh();
				ins15.Refresh();
				ins16.Refresh();
				ins17.Refresh();
				ins18.Refresh();
				ins19.Refresh();
//				ins20.Refresh();
//				ins21.Refresh();
//				ins22.Refresh();
//				ins23.Refresh();
//				ins24.Refresh();
//				ins25.Refresh();
//				ins26.Refresh();
//				ins27.Refresh();
//				ins28.Refresh();
//				ins29.Refresh();
//				ins30.Refresh();
//				ins31.Refresh();
//				ins32.Refresh();
//				ins33.Refresh();
//				ins34.Refresh();
//				ins35.Refresh();
//				ins36.Refresh();
//				ins37.Refresh();
//				ins38.Refresh();
//				ins39.Refresh();
//				ins40.Refresh();
//				ins41.Refresh();
//				ins42.Refresh();
//				ins43.Refresh();
//				ins44.Refresh();
//				ins45.Refresh();
//				ins46.Refresh();
//				ins47.Refresh();
//				ins48.Refresh();
//				ins49.Refresh();


				test.GlobalVNFinfo.add(ins0);
				test.GlobalVNFinfo.add(ins1);
				test.GlobalVNFinfo.add(ins2);
				test.GlobalVNFinfo.add(ins3);
				test.GlobalVNFinfo.add(ins4);
				test.GlobalVNFinfo.add(ins5);
				test.GlobalVNFinfo.add(ins6);
				test.GlobalVNFinfo.add(ins7);
				test.GlobalVNFinfo.add(ins8);
				test.GlobalVNFinfo.add(ins9);
				test.GlobalVNFinfo.add(ins10);
				test.GlobalVNFinfo.add(ins11);
				test.GlobalVNFinfo.add(ins12);
				test.GlobalVNFinfo.add(ins13);
				test.GlobalVNFinfo.add(ins14);
				test.GlobalVNFinfo.add(ins15);
				test.GlobalVNFinfo.add(ins16);
				test.GlobalVNFinfo.add(ins17);
				test.GlobalVNFinfo.add(ins18);
				test.GlobalVNFinfo.add(ins19);
//				test.GlobalVNFinfo.add(ins20);
//				test.GlobalVNFinfo.add(ins21);
//				test.GlobalVNFinfo.add(ins22);
//				test.GlobalVNFinfo.add(ins23);
//				test.GlobalVNFinfo.add(ins24);
//				test.GlobalVNFinfo.add(ins25);
//				test.GlobalVNFinfo.add(ins26);
//				test.GlobalVNFinfo.add(ins27);
//				test.GlobalVNFinfo.add(ins28);
//				test.GlobalVNFinfo.add(ins29);
//				test.GlobalVNFinfo.add(ins30);
//				test.GlobalVNFinfo.add(ins31);
//				test.GlobalVNFinfo.add(ins32);
//				test.GlobalVNFinfo.add(ins33);
//				test.GlobalVNFinfo.add(ins34);
//				test.GlobalVNFinfo.add(ins35);
//				test.GlobalVNFinfo.add(ins36);
//				test.GlobalVNFinfo.add(ins37);
//				test.GlobalVNFinfo.add(ins38);
//				test.GlobalVNFinfo.add(ins39);
//				test.GlobalVNFinfo.add(ins40);
//				test.GlobalVNFinfo.add(ins41);
//				test.GlobalVNFinfo.add(ins42);
//				test.GlobalVNFinfo.add(ins43);
//				test.GlobalVNFinfo.add(ins44);
//				test.GlobalVNFinfo.add(ins45);
//				test.GlobalVNFinfo.add(ins46);
//				test.GlobalVNFinfo.add(ins47);
//				test.GlobalVNFinfo.add(ins48);
//				test.GlobalVNFinfo.add(ins49);

				test.VNFNum = 20;
				long startTime = System.nanoTime();
//				try {
					Ratio[i][1] = test.LoadBalancingCheck();
//					System.out.println(Cost[i][1]);
//				} catch (LpSolveException e) {
//					e.printStackTrace();
//				}
				long endTime = System.nanoTime();
				Time[i][1] = (double) (endTime - startTime)/1000000.0;
//				System.out.println();
			}
			
//			System.out.println("**************************");
		}  //end for
		List<String> outputCost = new ArrayList<String>();
		List<String> outputTime = new ArrayList<String>();
		for (int i = 0; i < 20; i++){
			outputCost.add(Ratio[i][1]+",");
			outputTime.add(Time[i][1]+",");
		}
		System.out.println(CSVUtils.exportCsv(new File("/home/controller/ICNPAlgorithm/OFMAlgorithm/data/LB_Ratio50.csv"), outputCost));
		System.out.println(CSVUtils.exportCsv(new File("/home/controller/ICNPAlgorithm/OFMAlgorithm/data/LB_Time50.csv"), outputTime));
	}
}