
VertexShader {
  Buffers {
    ReadOnly Binding=0 {
      mat4 "ViewProjectionMatrix"
    }
  }

  Input {
    vec3 "Position" Location=0
    vec4 "Color" Location=1
  }

  Output {
    vec4 "OutColor" Location=0
  }

  Code Entry="main" {
    """
    void main()
    {
      gl_Position = ViewProjectionMatrix * vec4(Position, 1.0f);
      OutColor = Color;
    }
    """
  }
}

FragmentShader {
  sampler2D "Sampler" Binding=1

  Input {
    vec3 "Position" Location=0
    vec4 "Color" Location=1
  }

  Output {
    vec4 "OutColor" Location=0
  }

  Code Entry="main" {
    """
    void main()
    {
      FragmentColor = Color;
    }
    """
  }
}
