// automatically generated by the FlatBuffers compiler, do not modify

package MyGame;

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

@SuppressWarnings("unused")
public final class InParentNamespace extends Table {
  public static void ValidateVersion() { Constants.FLATBUFFERS_24_3_6(); }
  public static InParentNamespace getRootAsInParentNamespace(ByteBuffer _bb) { return getRootAsInParentNamespace(_bb, new InParentNamespace()); }
  public static InParentNamespace getRootAsInParentNamespace(ByteBuffer _bb, InParentNamespace obj) { _bb.order(ByteOrder.LITTLE_ENDIAN); return (obj.__assign(_bb.getInt(_bb.position()) + _bb.position(), _bb)); }
  public void __init(int _i, ByteBuffer _bb) { __reset(_i, _bb); }
  public InParentNamespace __assign(int _i, ByteBuffer _bb) { __init(_i, _bb); return this; }


  public static void startInParentNamespace(FlatBufferBuilder builder) { builder.startTable(0); }
  public static int endInParentNamespace(FlatBufferBuilder builder) {
    int o = builder.endTable();
    return o;
  }

  public static final class Vector extends BaseVector {
    public Vector __assign(int _vector, int _element_size, ByteBuffer _bb) { __reset(_vector, _element_size, _bb); return this; }

    public InParentNamespace get(int j) { return get(new InParentNamespace(), j); }
    public InParentNamespace get(InParentNamespace obj, int j) {  return obj.__assign(__indirect(__element(j), bb), bb); }
  }
  public InParentNamespaceT unpack() {
    InParentNamespaceT _o = new InParentNamespaceT();
    unpackTo(_o);
    return _o;
  }
  public void unpackTo(InParentNamespaceT _o) {
  }
  public static int pack(FlatBufferBuilder builder, InParentNamespaceT _o) {
    if (_o == null) return 0;
    startInParentNamespace(builder);
    return endInParentNamespace(builder);
  }
}

