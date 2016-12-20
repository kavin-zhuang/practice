import java.io.*;
import java.util.*;

public class EndianConvert {
    public static void main(String[] args) {
        // test print
        System.out.println("Hello World!");
        // test conversion
        byte[] data = new byte[2];
        byte[] tmp = new byte[data.length];
        data[0] = (byte)0xaa;
        data[1] = (byte)0x55;
        if(data.length > 1) {
            int i;
            for(i=data.length; i>0; i--) {
                tmp[i-1] = data[data.length-i];
            }
        }
        StringBuilder hexstream = new StringBuilder();
        for(byte b : tmp) {
            hexstream.append(String.format("%02X ", b));
        }
        System.out.println(hexstream.toString());
    }
}
