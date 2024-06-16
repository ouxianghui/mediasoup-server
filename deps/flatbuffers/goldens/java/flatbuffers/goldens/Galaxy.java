// automatically generated by the FlatBuffers compiler, do not modify

package flatbuffers.goldens;

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
public final class Galaxy extends Table {
  public static void ValidateVersion() { Constants.FLATBUFFERS_24_3_6(); }
  public static Galaxy getRootAsGalaxy(ByteBuffer _bb) { return getRootAsGalaxy(_bb, new Galaxy()); }
  public static Galaxy getRootAsGalaxy(ByteBuffer _bb, Galaxy obj) { _bb.order(ByteOrder.LITTLE_ENDIAN); return (obj.__assign(_bb.getInt(_bb.position()) + _bb.position(), _bb)); }
  public void __init(int _i, ByteBuffer _bb) { __reset(_i, _bb); }
  public Galaxy __assign(int _i, ByteBuffer _bb) { __init(_i, _bb); return this; }

  public long numStars() { int o = __offset(4); return o != 0 ? bb.getLong(o + bb_pos) : 0L; }

  public static int createGalaxy(FlatBufferBuilder builder,
      long numStars) {
    builder.startTable(1);
    Galaxy.addNumStars(builder, numStars);
    return Galaxy.endGalaxy(builder);
  }

  public static void startGalaxy(FlatBufferBuilder builder) { builder.startTable(1); }
  public static void addNumStars(FlatBufferBuilder builder, long numStars) { builder.addLong(0, numStars, 0L); }
  public static int endGalaxy(FlatBufferBuilder builder) {
    int o = builder.endTable();
    return o;
  }

  public static final class Vector extends BaseVector {
    public Vector __assign(int _vector, int _element_size, ByteBuffer _bb) { __reset(_vector, _element_size, _bb); return this; }

    public Galaxy get(int j) { return get(new Galaxy(), j); }
    public Galaxy get(Galaxy obj, int j) {  return obj.__assign(__indirect(__element(j), bb), bb); }
  }
}

