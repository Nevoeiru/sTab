package com.stab.sculptbridge

/**
 * Modelo de dados com a telemetria do pincel ativo no Blender.
 */
data class TelemetryData(
    val brushName: String = "Padrão",
    val size: Int = 50,
    val strength: Float = 0.5f,
    val symmetryX: Boolean = false
)

