
VertexShader {
  uniform "Globals" Binding=0 {
    mat4 "ViewProjectionMatrix"
  }

  Input {
    vec3 "VertexPosition" Location=0
    vec4 "VertexColor" Location=1
  }

  Output {
    vec4 "OutColor" Location=0
  }

  Code Entry="main" {
    `
    void main()
    {
      gl_Position = ViewProjectionMatrix * vec4(VertexPosition, 1.0f);
      OutColor = VertexColor;
    }
    `
  }
}

FragmentShader {
  Input {
    vec4 "Color" Location=0
  }

  Output {
    vec4 "FragmentColor" Location=0
  }

  Code Entry="main" {
    `
    void main()
    {
      FragmentColor = Color;
    }
    `
  }
}
