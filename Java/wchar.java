import java.io.*;
import java.util.*;

public class wchar {
    public static void main(String[] args) {
        // test print
        System.out.println("你好");
        // convert
	    String testA = new String("你好");
        System.out.println(testA);
        
        // test conversion
        byte[] data = new byte[4];
        data[0] = (byte)0x4F;
        data[1] = (byte)0x60;
        data[2] = (byte)0x59;
        data[3] = (byte)0x7D;
        try {
            String testB = new String(data, "unicode");
            System.out.println(testB);
        } catch (UnsupportedEncodingException e) {
            e.printStackTrace();
        }

	    // test conversion
        byte[] dataB = new byte[3];
        dataB[0] = (byte)0xe4;
        dataB[1] = (byte)0xbd;
        dataB[2] = (byte)0xa0;
        try {
            String testB = new String(dataB, "utf-8");
            System.out.println(testB);
        } catch (UnsupportedEncodingException e) {
            e.printStackTrace();
        }

	    // test conversion
        byte[] dataC = new byte[3];
        dataC[0] = (byte)0xc4;
        dataC[1] = (byte)0xe3;
	    dataC[2] = (byte)0x31;
        try {
            String testC = new String(dataC, "gbk");
            System.out.println(testC);
        } catch (UnsupportedEncodingException e) {
            e.printStackTrace();
        }

	    // test conversion
        byte[] dataD = new byte[2];
        dataD[0] = (byte)0x31;
        dataD[1] = (byte)0x32;
        try {
            String testD = new String(dataD, "gbk");
            System.out.println(testD);
        } catch (UnsupportedEncodingException e) {
            e.printStackTrace();
        }

	    // test conversion
        byte[] dataE = new byte[4];
	    dataE[0] = (byte)0x61;
        dataE[1] = (byte)0xc4;
        dataE[2] = (byte)0xe3;
	    dataE[3] = (byte)0x31;
        try {
            String testE = new String(dataE, "gbk");
            System.out.println(testE);
        } catch (UnsupportedEncodingException e) {
            e.printStackTrace();
        }
    }
}

