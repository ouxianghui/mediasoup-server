// automatically generated by the FlatBuffers compiler, do not modify

package MyGame.Example;

import com.google.flatbuffers.BaseVector;
import com.google.flatbuffers.BooleanVector;
import com.google.flatbuffers.ByteVector;
import com.google.flatbuffers.Constants;
import com.google.flatbuffers.DoubleVector;
import com.google.flatbuffers.FlatBufferBuilder;
import com.google.flatbuffers.FloatVector;
import com.google.flatbuffers.IntVector;
import com.google.flatbuffers.LongVector;
import com.google.flatbuffers.ShortVector;
import com.google.flatbuffers.StringVector;
import com.google.flatbuffers.Struct;
import com.google.flatbuffers.Table;
import com.google.flatbuffers.UnionVector;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

public class ArrayTableT {
  private MyGame.Example.ArrayStructT a;

  public MyGame.Example.ArrayStructT getA() { return a; }

  public void setA(MyGame.Example.ArrayStructT a) { this.a = a; }


  public ArrayTableT() {
    this.a = new MyGame.Example.ArrayStructT();
  }
  public static ArrayTableT deserializeFromBinary(byte[] fbBuffer) {
    return ArrayTable.getRootAsArrayTable(ByteBuffer.wrap(fbBuffer)).unpack();
  }
  public byte[] serializeToBinary() {
    FlatBufferBuilder fbb = new FlatBufferBuilder();
    ArrayTable.finishArrayTableBuffer(fbb, ArrayTable.pack(fbb, this));
    return fbb.sizedByteArray();
  }
}

