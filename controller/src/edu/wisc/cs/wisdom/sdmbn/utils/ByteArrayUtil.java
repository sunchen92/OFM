package edu.wisc.cs.wisdom.sdmbn.utils;

import java.util.Arrays;

public final class ByteArrayUtil
{
    private final byte[] data;

    public ByteArrayUtil(byte[] data)
    {
        if (data == null)
        {
            throw new NullPointerException();
        }
        this.data = data;
    }
    
    public byte[] getBytes()
    {
    	return this.data;
    }
    
    @Override
    public boolean equals(Object other)
    {
        if (!(other instanceof ByteArrayUtil))
        {
            return false;
        }
        return Arrays.equals(data, ((ByteArrayUtil)other).data);
    }

    @Override
    public int hashCode()
    {
        return Arrays.hashCode(data);
    }
}