bike_upgrades = {

  -- # When adding a new upgrade make sure that they fall into the category / types shown below
  -- #
  -- ## Category / Type ##
  -- #
  -- # "brakes" - Brakes and Tyres
  -- # 0 - Brakes
  -- # 1 - Tyre Soft
  -- # 2 - Tyre Hard
  -- #
  -- # "engine" - Engine
  -- # 0 - Balancing
  -- # 1 - Radiator
  -- # 2 - Crankshaft
  -- # 3 - Port Polish
  -- # 4 - Valves
  -- #
  -- # "exhaust" - Exhaust & Filters
  -- # 0 - Manifold
  -- # 1 - Exhaust
  -- # 2 - Air Filter
  -- # 3 - Oil Fuel
  -- #
  -- # "frame" - Frame & Suspension
  -- # 0 - Chassis
  -- # 1 - Fairing
  -- # 2 - Swing-arm
  -- # 3 - Shock Absorbers
  -- #
  -- # "gearbox" - GearBox
  -- # 0 - Clutch
  -- # 1 - Ratios
  -- #
  -- ## EFFECTS
  -- #
  -- # You can increment 8 different areas of the bike with a single upgrade
  -- # An effect will increment the current value of the tweak variable by the initial amount the other two set min and max values
  -- # Each level overwrites the previous one instead of accumulating

  -- # 125cc ---------------------------------------------------------------------------------------------------------------------

  -- # BRAKES

    brakes_125_stage_0 = {
    name = "",
    description = "",
    stage =0,
    staff_level =0,
    category = "brakes",
    type =0,
    time =0,
    championship = "c125",
    effects = {
[ "bike/data/body/Mass" ] = { "0.00750" },
[ "bike/data/brakes/0/Force" ] = { "-2.00000" },
[ "bike/data/brakes/1/Force" ] = { "-2.00000" },
[ "bike/data/NormalLoadEQ/DampingRate" ] = { "-0.05000" },
[ "bike/data/DrivingAids/FrontBrakeControl" ] = { "-0.28000" },
[ "bike/data/DrivingAids/FrontBrakeControlSetupMax" ] = { "-0.56000" },
[ "bike/data/DrivingAids/FrontBrakeControlSetupMin" ] = { "0.00000" },
[ "bike/data/DrivingAids/RearBrakeControl" ] = { "-0.28000" },
[ "bike/data/DrivingAids/RearBrakeControlSetupMax" ] = { "-0.56000" },
[ "bike/data/DrivingAids/RearBrakeControlSetupMin" ] = { "0.00000" }
    }
  },

    brakes_125_stage_1 = {
    name = [[FRENLER]],
    description = [[En uygun sıcaklığa daha çabuk ulaşan hafif diskler.]],
    stage =1,
    staff_level =2,
    category = "brakes",
    type =0,
    time =5,
    championship = "c125",
    effects = {
[ "bike/data/body/Mass" ] = { "0.00600" },
[ "bike/data/brakes/0/Force" ] = { "-1.60000" },
[ "bike/data/brakes/1/Force" ] = { "-1.60000" },
[ "bike/data/NormalLoadEQ/DampingRate" ] = { "-0.04000" },
[ "bike/data/DrivingAids/FrontBrakeControl" ] = { "-0.22400" },
[ "bike/data/DrivingAids/FrontBrakeControlSetupMax" ] = { "-0.44800" },
[ "bike/data/DrivingAids/FrontBrakeControlSetupMin" ] = { "0.00000" },
[ "bike/data/DrivingAids/RearBrakeControl" ] = { "-0.22400" },
[ "bike/data/DrivingAids/RearBrakeControlSetupMax" ] = { "-0.44800" },
[ "bike/data/DrivingAids/RearBrakeControlSetupMin" ] = { "0.00000" }
    }
  },

    brakes_125_stage_2 = {
    name = [[FRENLER]],
    description = [[Daha yüksek dayanıklılık ve frenleme gücü.]],
    stage =2,
    staff_level =4,
    category = "brakes",
    type =0,
    time =10,
    championship = "c125",
    effects = {
[ "bike/data/body/Mass" ] = { "0.00366" },
[ "bike/data/brakes/0/Force" ] = { "-0.97520" },
[ "bike/data/brakes/1/Force" ] = { "-0.97520" },
[ "bike/data/NormalLoadEQ/DampingRate" ] = { "-0.02438" },
[ "bike/data/DrivingAids/FrontBrakeControl" ] = { "-0.13653" },
[ "bike/data/DrivingAids/FrontBrakeControlSetupMax" ] = { "-0.27306" },
[ "bike/data/DrivingAids/FrontBrakeControlSetupMin" ] = { "0.00000" },
[ "bike/data/DrivingAids/RearBrakeControl" ] = { "-0.13653" },
[ "bike/data/DrivingAids/RearBrakeControlSetupMax" ] = { "-0.27306" },
[ "bike/data/DrivingAids/RearBrakeControlSetupMin" ] = { "0.00000" }
    }
  },

    brakes_125_stage_3 = {
    name = [[FRENLER]],
    description = [[Fren performansında ek iyileştirme.]],
    stage =3,
    staff_level =6,
    category = "brakes",
    type =0,
    time =15,
    championship = "c125",
    effects = {
[ "bike/data/body/Mass" ] = { "0.00000" },
[ "bike/data/brakes/0/Force" ] = { "0.00000" },
[ "bike/data/brakes/1/Force" ] = { "0.00000" },
[ "bike/data/NormalLoadEQ/DampingRate" ] = { "0.00000" },
[ "bike/data/DrivingAids/FrontBrakeControl" ] = { "0.00000" },
[ "bike/data/DrivingAids/FrontBrakeControlSetupMax" ] = { "0.00000" },
[ "bike/data/DrivingAids/FrontBrakeControlSetupMin" ] = { "0.00000" },
[ "bike/data/DrivingAids/RearBrakeControl" ] = { "0.00000" },
[ "bike/data/DrivingAids/RearBrakeControlSetupMax" ] = { "0.00000" },
[ "bike/data/DrivingAids/RearBrakeControlSetupMin" ] = { "0.00000" }
    }
  },

  -- # Soft tyres

    tyres_soft_compound_125_stage_0 = {
    name = "",
    description = "",
    stage =0,
    staff_level =0,
    category = "brakes",
    type =1,
    time =0,
    championship = "c125",
    effects = {
[ "bike/data/DrivingAids/TractionControl" ] = { "-0.20000" },
[ "bike/data/DrivingAids/TractionControlSetupMax" ] = { "-0.40000" },
[ "bike/data/DrivingAids/TractionControlSetupMin" ] = { "0.00000" },
[ "bike/data/wheels/0/Soft/LongitudinalFrictionCoeffMaxCoeff " ] = { "-0.20000" },
[ "bike/data/wheels/0/Soft/LateralFrictionMaxCoeff" ] = { "-0.20000" },
[ "bike/data/wheels/0/Soft/LateralFrictionCoeffLimitMaxCoeff" ] = { "-0.20000" },
[ "bike/data/wheels/1/Soft/LongitudinalFrictionCoeffMaxCoeff " ] = { "-0.20000" },
[ "bike/data/wheels/1/Soft/LateralFrictionMaxCoeff" ] = { "-0.20000" },
[ "bike/data/wheels/1/Soft/LateralFrictionCoeffLimitMaxCoeff" ] = { "-0.20000" }
    }
  },

    tyres_soft_compound_125_stage_1 = {
    name = [[YUMUŞAK HAMUR]],
    description = [[Yumuşak lastik hamurunun dayanıklılığını ve yol tutuşunu biraz iyileştirir.]],
    stage =1,
    staff_level =1,
    category = "brakes",
    type =1,
    time =5,
    championship = "c125",
    effects = {
[ "bike/data/DrivingAids/TractionControl" ] = { "-0.18000" },
[ "bike/data/DrivingAids/TractionControlSetupMax" ] = { "-0.36000" },
[ "bike/data/DrivingAids/TractionControlSetupMin" ] = { "0.00000" },
[ "bike/data/wheels/0/Soft/LongitudinalFrictionCoeffMaxCoeff " ] = { "-0.18000" },
[ "bike/data/wheels/0/Soft/LateralFrictionMaxCoeff" ] = { "-0.18000" },
[ "bike/data/wheels/0/Soft/LateralFrictionCoeffLimitMaxCoeff" ] = { "-0.18000" },
[ "bike/data/wheels/1/Soft/LongitudinalFrictionCoeffMaxCoeff " ] = { "-0.18000" },
[ "bike/data/wheels/1/Soft/LateralFrictionMaxCoeff" ] = { "-0.18000" },
[ "bike/data/wheels/1/Soft/LateralFrictionCoeffLimitMaxCoeff" ] = { "-0.18000" }
    }
  },

    tyres_soft_compound_125_stage_2 = {
    name = [[YUMUŞAK HAMUR]],
    description = [[Lastiklerin performansını daha uzun süre korumasını ve daha dayanıklı olmasını sağlayan yeni yumuşak hamur.]],
    stage =2,
    staff_level =4,
    category = "brakes",
    type =1,
    time =10,
    championship = "c125",
    effects = {
[ "bike/data/DrivingAids/TractionControl" ] = { "-0.14678" },
[ "bike/data/DrivingAids/TractionControlSetupMax" ] = { "-0.29356" },
[ "bike/data/DrivingAids/TractionControlSetupMin" ] = { "0.00000" },
[ "bike/data/wheels/0/Soft/LongitudinalFrictionCoeffMaxCoeff " ] = { "-0.14678" },
[ "bike/data/wheels/0/Soft/LateralFrictionMaxCoeff" ] = { "-0.14678" },
[ "bike/data/wheels/0/Soft/LateralFrictionCoeffLimitMaxCoeff" ] = { "-0.14678" },
[ "bike/data/wheels/1/Soft/LongitudinalFrictionCoeffMaxCoeff " ] = { "-0.14678" },
[ "bike/data/wheels/1/Soft/LateralFrictionMaxCoeff" ] = { "-0.14678" },
[ "bike/data/wheels/1/Soft/LateralFrictionCoeffLimitMaxCoeff" ] = { "-0.14678" }
    }
  },

    tyres_soft_compound_125_stage_3 = {
    name = [[YUMUŞAK HAMUR]],
    description = [[Yumuşak lastik hamurunun yol tutuşunda önemli iyileştirme.]],
    stage =3,
    staff_level =7,
    category = "brakes",
    type =1,
    time =15,
    championship = "c125",
    effects = {
[ "bike/data/DrivingAids/TractionControl" ] = { "-0.09160" },
[ "bike/data/DrivingAids/TractionControlSetupMax" ] = { "-0.18320" },
[ "bike/data/DrivingAids/TractionControlSetupMin" ] = { "0.00000" },
[ "bike/data/wheels/0/Soft/LongitudinalFrictionCoeffMaxCoeff " ] = { "-0.09160" },
[ "bike/data/wheels/0/Soft/LateralFrictionMaxCoeff" ] = { "-0.09160" },
[ "bike/data/wheels/0/Soft/LateralFrictionCoeffLimitMaxCoeff" ] = { "-0.09160" },
[ "bike/data/wheels/1/Soft/LongitudinalFrictionCoeffMaxCoeff " ] = { "-0.09160" },
[ "bike/data/wheels/1/Soft/LateralFrictionMaxCoeff" ] = { "-0.09160" },
[ "bike/data/wheels/1/Soft/LateralFrictionCoeffLimitMaxCoeff" ] = { "-0.09160" }
    }
  },

    tyres_soft_compound_125_stage_4 = {
    name = [[YUMUŞAK HAMUR]],
    description = [[Yumuşak lastik hamurunun dayanıklılığını artırır.]],
    stage =4,
    staff_level =10,
    category = "brakes",
    type =1,
    time =20,
    championship = "c125",
    effects = {
[ "bike/data/DrivingAids/TractionControl" ] = { "0.00000" },
[ "bike/data/DrivingAids/TractionControlSetupMax" ] = { "0.00000" },
[ "bike/data/DrivingAids/TractionControlSetupMin" ] = { "0.00000" },
[ "bike/data/wheels/0/Soft/LongitudinalFrictionCoeffMaxCoeff " ] = { "0.00000" },
[ "bike/data/wheels/0/Soft/LateralFrictionMaxCoeff" ] = { "0.00000" },
[ "bike/data/wheels/0/Soft/LateralFrictionCoeffLimitMaxCoeff" ] = { "0.00000" },
[ "bike/data/wheels/1/Soft/LongitudinalFrictionCoeffMaxCoeff " ] = { "0.00000" },
[ "bike/data/wheels/1/Soft/LateralFrictionMaxCoeff" ] = { "0.00000" },
[ "bike/data/wheels/1/Soft/LateralFrictionCoeffLimitMaxCoeff" ] = { "0.00000" }
    }
  },

    -- # Hard tyres

    tyres_hard_compound_125_stage_0 = {
    name = "",
    description = "",
    stage =0,
    staff_level =0,
    category = "brakes",
    type =2,
    time =0,
    championship = "c125",
    effects = {
[ "bike/data/DrivingAids/TractionControl" ] = { "-0.20000" },
[ "bike/data/DrivingAids/TractionControlSetupMax" ] = { "-0.40000" },
[ "bike/data/DrivingAids/TractionControlSetupMin" ] = { "0.00000" },
[ "bike/data/wheels/0/Hard/LongitudinalFrictionCoeffMaxCoeff " ] = { "-0.20000" },
[ "bike/data/wheels/0/Hard/LateralFrictionMaxCoeff" ] = { "-0.20000" },
[ "bike/data/wheels/0/Hard/LateralFrictionCoeffLimitMaxCoeff" ] = { "-0.20000" },
[ "bike/data/wheels/1/Hard/LongitudinalFrictionCoeffMaxCoeff " ] = { "-0.20000" },
[ "bike/data/wheels/1/Hard/LateralFrictionMaxCoeff" ] = { "-0.20000" },
[ "bike/data/wheels/1/Hard/LateralFrictionCoeffLimitMaxCoeff" ] = { "-0.20000" }
    }
  },

    tyres_hard_compound_125_stage_1 = {
    name = [[SERT HAMUR]],
    description = [[Sert lastik hamurunun dayanıklılığını ve yol tutuşunu biraz iyileştirir.]],
    stage =1,
    staff_level =1,
    category = "brakes",
    type =2,
    time =5,
    championship = "c125",
    effects = {
[ "bike/data/DrivingAids/TractionControl" ] = { "-0.18000" },
[ "bike/data/DrivingAids/TractionControlSetupMax" ] = { "-0.36000" },
[ "bike/data/DrivingAids/TractionControlSetupMin" ] = { "0.00000" },
[ "bike/data/wheels/0/Hard/LongitudinalFrictionCoeffMaxCoeff " ] = { "-0.18000" },
[ "bike/data/wheels/0/Hard/LateralFrictionMaxCoeff" ] = { "-0.18000" },
[ "bike/data/wheels/0/Hard/LateralFrictionCoeffLimitMaxCoeff" ] = { "-0.18000" },
[ "bike/data/wheels/1/Hard/LongitudinalFrictionCoeffMaxCoeff " ] = { "-0.18000" },
[ "bike/data/wheels/1/Hard/LateralFrictionMaxCoeff" ] = { "-0.18000" },
[ "bike/data/wheels/1/Hard/LateralFrictionCoeffLimitMaxCoeff" ] = { "-0.18000" }
    }
  },

    tyres_hard_compound_125_stage_2 = {
    name = [[SERT HAMUR]],
    description = [[Lastiklerin performansını daha uzun süre korumasını ve daha dayanıklı olmasını sağlayan yeni sert hamur.]],
    stage =2,
    staff_level =4,
    category = "brakes",
    type =2,
    time =10,
    championship = "c125",
    effects = {
[ "bike/data/DrivingAids/TractionControl" ] = { "-0.14678" },
[ "bike/data/DrivingAids/TractionControlSetupMax" ] = { "-0.29356" },
[ "bike/data/DrivingAids/TractionControlSetupMin" ] = { "0.00000" },
[ "bike/data/wheels/0/Hard/LongitudinalFrictionCoeffMaxCoeff " ] = { "-0.14678" },
[ "bike/data/wheels/0/Hard/LateralFrictionMaxCoeff" ] = { "-0.14678" },
[ "bike/data/wheels/0/Hard/LateralFrictionCoeffLimitMaxCoeff" ] = { "-0.14678" },
[ "bike/data/wheels/1/Hard/LongitudinalFrictionCoeffMaxCoeff " ] = { "-0.14678" },
[ "bike/data/wheels/1/Hard/LateralFrictionMaxCoeff" ] = { "-0.14678" },
[ "bike/data/wheels/1/Hard/LateralFrictionCoeffLimitMaxCoeff" ] = { "-0.14678" }
    }
  },

    tyres_hard_compound_125_stage_3 = {
    name = [[SERT HAMUR]],
    description = [[Sert lastik hamurunun yol tutuşunda önemli iyileştirme.]],
    stage =3,
    staff_level =7,
    category = "brakes",
    type =2,
    time =15,
    championship = "c125",
    effects = {
[ "bike/data/DrivingAids/TractionControl" ] = { "-0.09160" },
[ "bike/data/DrivingAids/TractionControlSetupMax" ] = { "-0.18320" },
[ "bike/data/DrivingAids/TractionControlSetupMin" ] = { "0.00000" },
[ "bike/data/wheels/0/Hard/LongitudinalFrictionCoeffMaxCoeff " ] = { "-0.09160" },
[ "bike/data/wheels/0/Hard/LateralFrictionMaxCoeff" ] = { "-0.09160" },
[ "bike/data/wheels/0/Hard/LateralFrictionCoeffLimitMaxCoeff" ] = { "-0.09160" },
[ "bike/data/wheels/1/Hard/LongitudinalFrictionCoeffMaxCoeff " ] = { "-0.09160" },
[ "bike/data/wheels/1/Hard/LateralFrictionMaxCoeff" ] = { "-0.09160" },
[ "bike/data/wheels/1/Hard/LateralFrictionCoeffLimitMaxCoeff" ] = { "-0.09160" }
    }
  },

    tyres_hard_compound_125_stage_4 = {
    name = [[SERT HAMUR]],
    description = [[Sert lastik hamurunun dayanıklılığını artırır.]],
    stage =4,
    staff_level =10,
    category = "brakes",
    type =2,
    time =20,
    championship = "c125",
    effects = {
[ "bike/data/DrivingAids/TractionControl" ] = { "0.00000" },
[ "bike/data/DrivingAids/TractionControlSetupMax" ] = { "0.00000" },
[ "bike/data/DrivingAids/TractionControlSetupMin" ] = { "0.00000" },
[ "bike/data/wheels/0/Hard/LongitudinalFrictionCoeffMaxCoeff " ] = { "0.00000" },
[ "bike/data/wheels/0/Hard/LateralFrictionMaxCoeff" ] = { "0.00000" },
[ "bike/data/wheels/0/Hard/LateralFrictionCoeffLimitMaxCoeff" ] = { "0.00000" },
[ "bike/data/wheels/1/Hard/LongitudinalFrictionCoeffMaxCoeff " ] = { "0.00000" },
[ "bike/data/wheels/1/Hard/LateralFrictionMaxCoeff" ] = { "0.00000" },
[ "bike/data/wheels/1/Hard/LateralFrictionCoeffLimitMaxCoeff" ] = { "0.00000" }
    }
  },

  -- # ENGINE

  -- # ENGINE - BALANCING

    engine_balancing_125_stage_0 = {
    name = "",
    description = "",
    stage =0,
    staff_level =0,
    category = "engine",
    type =0,
    time =0,
    championship = "c125",
    effects = {
[ "bike/data/engine/MaxTorque" ] = { "-0.01200" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00240" }
    }
  },

    engine_balancing_125_stage_1 = {
    name = [[BALANS AYARI]],
    description = [[Motorun hassas biçimde dengelenmesi; aşırı ses, titreşim ve sürtünmeden kaynaklanan enerji kaybını azaltarak verimliliği artırır.]],
    stage =1,
    staff_level =8,
    category = "engine",
    type =0,
    time =15,
    championship = "c125",
    effects = {
[ "bike/data/engine/MaxTorque" ] = { "0.00000" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "0.00000" }
    }
  },

  -- # ENGINE - RADIATOR

    engine_radiator_125_stage_0 = {
    name = "",
    description = "",
    stage =0,
    staff_level =0,
    category = "engine",
    type =1,
    time =0,
    championship = "c125",
    effects = {
[ "bike/data/aerodynamic/BikeDragCoeff" ] = { "0.00600" },
[ "bike/data/aerodynamic/BikeReferenceArea" ] = { "0.00600" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00240" },
[ "bike/data/engine/MaxTorque" ] = { "-0.01200" }
    }
  },

    engine_radiator_125_stage_1 = {
    name = [[RADYATÖR]],
    description = [[Radyatörün hava girişleri iyileştirilerek hava direnci azaltılır ve havalandırma artırılır.]],
    stage =1,
    staff_level =1,
    category = "engine",
    type =1,
    time =5,
    championship = "c125",
    effects = {
[ "bike/data/aerodynamic/BikeDragCoeff" ] = { "0.00300" },
[ "bike/data/aerodynamic/BikeReferenceArea" ] = { "0.00300" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00120" },
[ "bike/data/engine/MaxTorque" ] = { "-0.00600" }
    }
  },

    engine_radiator_125_stage_2 = {
    name = [[RADYATÖR]],
    description = [[Radyatörün aerodinamiğini ve motoru soğutma performansını daha da iyileştirir.]],
    stage =2,
    staff_level =7,
    category = "engine",
    type =1,
    time =15,
    championship = "c125",
    effects = {
[ "bike/data/aerodynamic/BikeDragCoeff" ] = { "0.00000" },
[ "bike/data/aerodynamic/BikeReferenceArea" ] = { "0.00000" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "0.00000" },
[ "bike/data/engine/MaxTorque" ] = { "0.00000" }
    }
  },

  -- # ENGINE - CRANKSHAFT

    engine_crankshaft_125_stage_0 = {
    name = "",
    description = "",
    stage =0,
    staff_level =0,
    category = "engine",
    type =2,
    time =0,
    championship = "c125",
    effects = {
[ "bike/data/engine/MaxTorque" ] = { "0.00600" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00240" },
[ "bike/data/lean/rate/TorqueMax" ] = { "-0.02460" },
[ "bike/data/lean/rate/Damping" ] = { "0.05000" },
[ "bike/data/lean/angle/SteerBias" ] = { "0.00000" }
    }
  },

    engine_crankshaft_125_stage_1 = {
    name = [[KRANK MİLİ]],
    description = [[Motosiklet üzerindeki jiroskopik kuvvetleri dengelemek için tekerleklerin ters yönünde dönen bir krank mili sistemi geliştirir. Bir miktar motor gücünden ödün vererek motosikletin viraj tepkisini iyileştirir.]],
    stage =1,
    staff_level =12,
    category = "engine",
    type =2,
    time =25,
    championship = "c125",
    effects = {
[ "bike/data/engine/MaxTorque" ] = { "0.00000" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "0.00000" },
[ "bike/data/lean/rate/TorqueMax" ] = { "0.00000" },
[ "bike/data/lean/rate/Damping" ] = { "0.00000" },
[ "bike/data/lean/angle/SteerBias" ] = { "0.00000" }
    }
  },

  -- # ENGINE - PORT POLISH

    engine_portpolish_125_stage_0 = {
    name = "",
    description = "",
    stage =0,
    staff_level =0,
    category = "engine",
    type =3,
    time =0,
    championship = "c125",
    effects = {
[ "bike/data/engine/MaxTorque" ] = { "-0.00600" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00150" }
    }
  },

    engine_portpolish_125_stage_1 = {
    name = [[KANAL PARLATMA]],
    description = [[Motorun emme kanalları parlatılarak hava akışı düzgünleştirilir.]],
    stage =1,
    staff_level =8,
    category = "engine",
    type =3,
    time =10,
    championship = "c125",
    effects = {
[ "bike/data/engine/MaxTorque" ] = { "0.00000" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "0.00000" }
    }
  },

  -- # ENGINE - VALVES

    engine_valves_125_stage_0 = {
    name = "",
    description = "",
    stage =0,
    staff_level =0,
    category = "engine",
    type =4,
    time =0,
    championship = "c125",
    effects = {
[ "bike/data/engine/MaxTorque" ] = { "-0.03000" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00540" },
[ "bike/data/DrivingAids/TractionControl" ] = { "-0.20000" },
[ "bike/data/DrivingAids/TractionControlSetupMax" ] = { "-0.40000" },
[ "bike/data/DrivingAids/TractionControlSetupMin" ] = { "0.00000" }
    }
  },

    engine_valves_125_stage_1 = {
    name = [[SUPAPLAR]],
    description = [[Daha güvenilir yaylı supaplar geliştirir.]],
    stage =1,
    staff_level =2,
    category = "engine",
    type =4,
    time =10,
    championship = "c125",
    effects = {
[ "bike/data/engine/MaxTorque" ] = { "-0.02400" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00432" },
[ "bike/data/DrivingAids/TractionControl" ] = { "-0.16000" },
[ "bike/data/DrivingAids/TractionControlSetupMax" ] = { "-0.32000" },
[ "bike/data/DrivingAids/TractionControlSetupMin" ] = { "0.00000" }
    }
  },

    engine_valves_125_stage_2 = {
    name = [[SUPAPLAR]],
    description = [[Gelişmiş silindir kapağı teknolojisi. Yayların yerini külbütör mekanizmaları alır. Motor performansı artar.]],
    stage =2,
    staff_level =6,
    category = "engine",
    type =4,
    time =15,
    championship = "c125",
    effects = {
[ "bike/data/engine/MaxTorque" ] = { "-0.01463" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00263" },
[ "bike/data/DrivingAids/TractionControl" ] = { "-0.09752" },
[ "bike/data/DrivingAids/TractionControlSetupMax" ] = { "-0.19504" },
[ "bike/data/DrivingAids/TractionControlSetupMin" ] = { "0.00000" }
    }
  },

    engine_valves_125_stage_3 = {
    name = [[SUPAPLAR]],
    description = [[Supapların açılma ve kapanma hızı artırılarak motor gücü yükseltilir.]],
    stage =3,
    staff_level =10,
    category = "engine",
    type =4,
    time =20,
    championship = "c125",
    effects = {
[ "bike/data/engine/MaxTorque" ] = { "0.00000" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "0.00000" },
[ "bike/data/DrivingAids/TractionControl" ] = { "0.00000" },
[ "bike/data/DrivingAids/TractionControlSetupMax" ] = { "0.00000" },
[ "bike/data/DrivingAids/TractionControlSetupMin" ] = { "0.00000" }
    }
  },

  -- # EXHAUST - MANIFOLD

    exhaust_manifold_125_stage_0 = {
    name = "",
    description = "",
    stage =0,
    staff_level =0,
    category = "exhaust",
    type =0,
    time =0,
    championship = "c125",
    effects = {
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00450" },
[ "bike/data/engine/MaxTorque" ] = { "-0.01920" },
[ "bike/data/body/Mass" ] = { "0.01500" }
    }
  },

    exhaust_manifold_125_stage_1 = {
    name = [[MANİFOLD]],
    description = [[Manifold, daha sağlam ve hafif malzemelerden yapılmış bir modelle değiştirilir.]],
    stage =1,
    staff_level =7,
    category = "exhaust",
    type =0,
    time =10,
    championship = "c125",
    effects = {
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00360" },
[ "bike/data/engine/MaxTorque" ] = { "-0.01536" },
[ "bike/data/body/Mass" ] = { "0.01200" }
    }
  },

    exhaust_manifold_125_stage_2 = {
    name = [[MANİFOLD]],
    description = [[İkinci aşama geliştirmesi, daha verimli hava akışı için manifoldu motorun ateşleme sırasına göre ayarlar.]],
    stage =2,
    staff_level =9,
    category = "exhaust",
    type =0,
    time =15,
    championship = "c125",
    effects = {
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00219" },
[ "bike/data/engine/MaxTorque" ] = { "-0.00936" },
[ "bike/data/body/Mass" ] = { "0.00731" }
    }
  },

    exhaust_manifold_125_stage_3 = {
    name = [[MANİFOLD]],
    description = [[Bu geliştirme, hava akışındaki basınç değişimlerinden yararlanarak gazları motorun içinden tam zamanında geçirmek için manifoldu daha da iyileştirir.]],
    stage =3,
    staff_level =11,
    category = "exhaust",
    type =0,
    time =20,
    championship = "c125",
    effects = {
[ "bike/data/gearbox/DifferentialRatio" ] = { "0.00000" },
[ "bike/data/engine/MaxTorque" ] = { "0.00000" },
[ "bike/data/body/Mass" ] = { "0.00000" }
    }
  },

  -- # EXHAUST - EXHAUST

    exhaust_exhaust_125_stage_0 = {
    name = "",
    description = "",
    stage =0,
    staff_level =0,
    category = "exhaust",
    type =1,
    time =0,
    championship = "c125",
    effects = {
[ "bike/data/body/Mass" ] = { "0.01500" },
[ "bike/data/engine/MaxTorque" ] = { "-0.00600" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00120" },
[ "bike/data/lean/rate/TorqueMax" ] = { "-0.00410" },
[ "bike/data/lean/rate/Damping" ] = { "0.05000" }
    }
  },

    exhaust_exhaust_125_stage_1 = {
    name = [[EGZOZ]],
    description = [[Motosiklete daha hafif ve sağlam yeni bir egzoz sistemi takılır.]],
    stage =1,
    staff_level =4,
    category = "exhaust",
    type =1,
    time =5,
    championship = "c125",
    effects = {
[ "bike/data/body/Mass" ] = { "0.00750" },
[ "bike/data/engine/MaxTorque" ] = { "-0.00300" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00060" },
[ "bike/data/lean/rate/TorqueMax" ] = { "-0.00205" },
[ "bike/data/lean/rate/Damping" ] = { "0.02501" }
    }
  },

    exhaust_exhaust_125_stage_2 = {
    name = [[EGZOZ]],
    description = [[Egzozun hava akışında ve dayanıklılığında küçük iyileştirmeler.]],
    stage =2,
    staff_level =8,
    category = "exhaust",
    type =1,
    time =10,
    championship = "c125",
    effects = {
[ "bike/data/body/Mass" ] = { "0.00000" },
[ "bike/data/engine/MaxTorque" ] = { "0.00000" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "0.00000" },
[ "bike/data/lean/rate/TorqueMax" ] = { "0.00000" },
[ "bike/data/lean/rate/Damping" ] = { "0.00000" }
    }
  },

  -- # EXHAUST - AIR

    exhaust_air_125_stage_0 = {
    name = "",
    description = "",
    stage =0,
    staff_level =0,
    category = "exhaust",
    type =2,
    time =0,
    championship = "c125",
    effects = {
[ "bike/data/engine/MaxTorque" ] = { "-0.01200" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00300" }
    }
  },

    exhaust_air_125_stage_1 = {
    name = [[HAVA FİLTRESİ]],
    description = [[Hava filtresi, kirin motora ulaşmasını engelleyen daha dayanıklı ve verimli bir modelle değiştirilir.]],
    stage =1,
    staff_level =6,
    category = "exhaust",
    type =2,
    time =10,
    championship = "c125",
    effects = {
[ "bike/data/engine/MaxTorque" ] = { "0.00000" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "0.00000" }
    }
  },

  -- # EXHAUST - OIL / FUEL

  exhaust_oilfuel_125_stage_0 = {
    name = "",
    description = "",
    stage =0,
    staff_level =0,
    category = "exhaust",
    type =3,
    time =0,
    championship = "c125",
    effects = {
[ "bike/data/engine/MaxTorque" ] = { "-0.01680" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00420" }
    }
  },

    exhaust_oilfuel_125_stage_1 = {
    name = [[YAĞ/YAKIT]],
    description = [[Motorda kullanılan yağın kalitesini yükselterek sürtünmeyi azaltır.]],
    stage =1,
    staff_level =1,
    category = "exhaust",
    type =3,
    time =5,
    championship = "c125",
    effects = {
[ "bike/data/engine/MaxTorque" ] = { "-0.00840" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00210" }
    }
  },

    exhaust_oilfuel_125_stage_2 = {
    name = [[YAĞ/YAKIT]],
    description = [[Daha fazla enerji açığa çıkaran ve daha yüksek güç sağlayan, yüksek kaliteli ve oktanı artırılmış yakıt geliştirir.]],
    stage =2,
    staff_level =4,
    category = "exhaust",
    type =3,
    time =5,
    championship = "c125",
    effects = {
[ "bike/data/engine/MaxTorque" ] = { "0.00000" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "0.00000" }
    }
  },

  -- # FRAME - CHASSIS

    frame_chassis_125_stage_0 = {
    name = "",
    description = "",
    stage =0,
    staff_level =0,
    category = "frame",
    type =0,
    time =0,
    championship = "c125",
    effects = {
[ "bike/data/body/Mass" ] = { "0.03000" },
[ "bike/data/lean/rate/TorqueMax" ] = { "-0.03280" },
[ "bike/data/lean/rate/Damping" ] = { "0.20000" },
[ "bike/data/lean/angle/SteerBias" ] = { "0.00000" }
    }
  },

    frame_chassis_125_stage_1 = {
    name = [[ŞASİ]],
    description = [[Şasi yapımı için hem sağlam hem hafif yeni malzemeler geliştirir.]],
    stage =1,
    staff_level =3,
    category = "frame",
    type =0,
    time =10,
    championship = "c125",
    effects = {
[ "bike/data/body/Mass" ] = { "0.01501" },
[ "bike/data/lean/rate/TorqueMax" ] = { "-0.01641" },
[ "bike/data/lean/rate/Damping" ] = { "0.10004" },
[ "bike/data/lean/angle/SteerBias" ] = { "0.00000" }
    }
  },

    frame_chassis_125_stage_2 = {
    name = [[ŞASİ]],
    description = [[Ağırlık daha da azaltılarak hızlanma ve son hız önemli ölçüde iyileştirilir.]],
    stage =2,
    staff_level =7,
    category = "frame",
    type =0,
    time =15,
    championship = "c125",
    effects = {
[ "bike/data/body/Mass" ] = { "0.00000" },
[ "bike/data/lean/rate/TorqueMax" ] = { "0.00000" },
[ "bike/data/lean/rate/Damping" ] = { "0.00000" },
[ "bike/data/lean/angle/SteerBias" ] = { "0.00000" }
    }
  },

  -- # FRAME - FAIRING

    frame_fairing_125_stage_0 = {
    name = "",
    description = "",
    stage =0,
    staff_level =0,
    category = "frame",
    type =1,
    time =0,
    championship = "c125",
    effects = {
[ "bike/data/aerodynamic/BikeDragCoeff" ] = { "0.03400" },
[ "bike/data/aerodynamic/BikeReferenceArea" ] = { "0.03400" },
[ "bike/data/aerodynamic/RiderReferenceArea" ] = { "0.04000" }
    }
  },

    frame_fairing_125_stage_1 = {
    name = [[GRENAJ]],
    description = [[Grenaj ve motosiklet biçimindeki temel iyileştirmeler, sürücü eğildiğinde havanın sırtı üzerinden daha düzgün akmasını sağlar.]],
    stage =1,
    staff_level =1,
    category = "frame",
    type =1,
    time =5,
    championship = "c125",
    effects = {
[ "bike/data/aerodynamic/BikeDragCoeff" ] = { "0.02720" },
[ "bike/data/aerodynamic/BikeReferenceArea" ] = { "0.02720" },
[ "bike/data/aerodynamic/RiderReferenceArea" ] = { "0.03200" }
    }
  },

    frame_fairing_125_stage_2 = {
    name = [[GRENAJ]],
    description = [[Grenajdaki orta düzey değişiklikler, motosikletin önemli bölgelerindeki aerodinamik verimliliği artırır.]],
    stage =2,
    staff_level =5,
    category = "frame",
    type =1,
    time =10,
    championship = "c125",
    effects = {
[ "bike/data/aerodynamic/BikeDragCoeff" ] = { "0.01658" },
[ "bike/data/aerodynamic/BikeReferenceArea" ] = { "0.01658" },
[ "bike/data/aerodynamic/RiderReferenceArea" ] = { "0.01950" }
    }
  },

    frame_fairing_125_stage_3 = {
    name = [[GRENAJ]],
    description = [[Bileşenlerin aşırı ısınmasını önlemek için ısı dağılımına odaklanan ek grenaj iyileştirmeleri.]],
    stage =3,
    staff_level =9,
    category = "frame",
    type =1,
    time =15,
    championship = "c125",
    effects = {
[ "bike/data/aerodynamic/BikeDragCoeff" ] = { "0.00000" },
[ "bike/data/aerodynamic/BikeReferenceArea" ] = { "0.00000" },
[ "bike/data/aerodynamic/RiderReferenceArea" ] = { "0.00000" }
    }
  },

  -- # FRAME - SWING ARM

    frame_swingarm_125_stage_0 = {
    name = "",
    description = "",
    stage =0,
    staff_level =0,
    category = "frame",
    type =2,
    time =0,
    championship = "c125",
    effects = {
[ "bike/data/DrivingAids/RearBrakeControl" ] = { "-0.12000" },
[ "bike/data/DrivingAids/RearBrakeControlSetupMax" ] = { "-0.24000" },
[ "bike/data/DrivingAids/RearBrakeControlSetupMin" ] = { "0.00000" },
[ "bike/data/lean/rate/TorqueMax" ] = { "-0.01230" },
[ "bike/data/lean/rate/Damping" ] = { "0.10000" },
[ "bike/data/lean/angle/SteerBias" ] = { "0.00000" },
[ "bike/data/NormalLoadEQ/DampingRate" ] = { "-0.10000" },
[ "bike/data/suspensions/1/Stiffness" ] = { "5.00000" },
[ "bike/data/suspensions/1/StiffnessSetupMax" ] = { "5.00000" },
[ "bike/data/suspensions/1/StiffnessSetupMin" ] = { "5.00000" },
[ "bike/data/suspensions/1/CompressionDamping" ] = { "-0.15000" },
[ "bike/data/suspensions/1/CompressionDampingSetupMax" ] = { "-0.15000" },
[ "bike/data/suspensions/1/CompressionDampingSetupMin" ] = { "-0.15000" },
[ "bike/data/suspensions/1/ReboundDamping" ] = { "-0.10000" },
[ "bike/data/suspensions/1/ReboundDampingSetupMin" ] = { "-0.10000" },
[ "bike/data/suspensions/1/ReboundDampingSetupMax" ] = { "-0.10000" }
    }
  },

    frame_swingarm_125_stage_1 = {
    name = [[SALINCAK]],
    description = [[Salıncak, zincir torkunun oluşturduğu çok büyük baskı kuvvetlerine dayanmalıdır. Daha sağlam malzemeler kullanılması salıncağın dayanıklılığını artırır.]],
    stage =1,
    staff_level =5,
    category = "frame",
    type =2,
    time =10,
    championship = "c125",
    effects = {
[ "bike/data/DrivingAids/RearBrakeControl" ] = { "-0.06002" },
[ "bike/data/DrivingAids/RearBrakeControlSetupMax" ] = { "-0.12005" },
[ "bike/data/DrivingAids/RearBrakeControlSetupMin" ] = { "0.00000" },
[ "bike/data/lean/rate/TorqueMax" ] = { "-0.00615" },
[ "bike/data/lean/rate/Damping" ] = { "0.05002" },
[ "bike/data/lean/angle/SteerBias" ] = { "0.00000" },
[ "bike/data/NormalLoadEQ/DampingRate" ] = { "-0.05002" },
[ "bike/data/suspensions/1/Stiffness" ] = { "2.50100" },
[ "bike/data/suspensions/1/StiffnessSetupMax" ] = { "2.50100" },
[ "bike/data/suspensions/1/StiffnessSetupMin" ] = { "2.50100" },
[ "bike/data/suspensions/1/CompressionDamping" ] = { "-0.07503" },
[ "bike/data/suspensions/1/CompressionDampingSetupMax" ] = { "-0.07503" },
[ "bike/data/suspensions/1/CompressionDampingSetupMin" ] = { "-0.07503" },
[ "bike/data/suspensions/1/ReboundDamping" ] = { "-0.05002" },
[ "bike/data/suspensions/1/ReboundDampingSetupMin" ] = { "-0.05002" },
[ "bike/data/suspensions/1/ReboundDampingSetupMax" ] = { "-0.05002" }
    }
  },

    frame_swingarm_125_stage_2 = {
    name = [[SALINCAK]],
    description = [[Salıncak süspansiyonundaki iyileştirmeler, viraj çıkışında hızlanırken daha yüksek kontrol sağlar.]],
    stage =2,
    staff_level =8,
    category = "frame",
    type =2,
    time =10,
    championship = "c125",
    effects = {
[ "bike/data/DrivingAids/RearBrakeControl" ] = { "0.00000" },
[ "bike/data/DrivingAids/RearBrakeControlSetupMax" ] = { "0.00000" },
[ "bike/data/DrivingAids/RearBrakeControlSetupMin" ] = { "0.00000" },
[ "bike/data/lean/rate/TorqueMax" ] = { "0.00000" },
[ "bike/data/lean/rate/Damping" ] = { "0.00000" },
[ "bike/data/lean/angle/SteerBias" ] = { "0.00000" },
[ "bike/data/NormalLoadEQ/DampingRate" ] = { "0.00000" },
[ "bike/data/suspensions/1/Stiffness" ] = { "0.00000" },
[ "bike/data/suspensions/1/StiffnessSetupMax" ] = { "0.00000" },
[ "bike/data/suspensions/1/StiffnessSetupMin" ] = { "0.00000" },
[ "bike/data/suspensions/1/CompressionDamping" ] = { "0.00000" },
[ "bike/data/suspensions/1/CompressionDampingSetupMax" ] = { "0.00000" },
[ "bike/data/suspensions/1/CompressionDampingSetupMin" ] = { "0.00000" },
[ "bike/data/suspensions/1/ReboundDamping" ] = { "0.00000" },
[ "bike/data/suspensions/1/ReboundDampingSetupMin" ] = { "0.00000" },
[ "bike/data/suspensions/1/ReboundDampingSetupMax" ] = { "0.00000" }
    }
  },

  -- # FRAME - SHOCK ABSORBERS

    frame_shock_absorbers_125_stage_0 = {
    name = "",
    description = "",
    stage =0,
    staff_level =0,
    category = "frame",
    type =3,
    time =0,
    championship = "c125",
    effects = {
[ "bike/data/DrivingAids/FrontBrakeControl" ] = { "-0.12000" },
[ "bike/data/DrivingAids/FrontBrakeControlSetupMax" ] = { "-0.24000" },
[ "bike/data/DrivingAids/FrontBrakeControlSetupMin" ] = { "0.00000" },
[ "bike/data/lean/rate/TorqueMax" ] = { "-0.00820" },
[ "bike/data/lean/rate/Damping" ] = { "0.10000" },
[ "bike/data/lean/angle/SteerBias" ] = { "0.00000" },
[ "bike/data/NormalLoadEQ/DampingRate" ] = { "-0.35000" },
[ "bike/data/suspensions/0/Stiffness" ] = { "10.00000" },
[ "bike/data/suspensions/0/StiffnessSetupMax" ] = { "10.00000" },
[ "bike/data/suspensions/0/StiffnessSetupMin" ] = { "10.00000" },
[ "bike/data/suspensions/0/CompressionDamping" ] = { "-0.30000" },
[ "bike/data/suspensions/0/CompressionDampingSetupMax" ] = { "-0.50000" },
[ "bike/data/suspensions/0/CompressionDampingSetupMin" ] = { "-0.30000" },
[ "bike/data/suspensions/0/ReboundDamping" ] = { "-0.20000" },
[ "bike/data/suspensions/0/ReboundDampingSetupMax" ] = { "-0.50000" },
[ "bike/data/suspensions/0/ReboundDampingSetupMin" ] = { "-0.20000" },
[ "bike/data/suspensions/1/Stiffness" ] = { "5.00000" },
[ "bike/data/suspensions/1/StiffnessSetupMax" ] = { "5.00000" },
[ "bike/data/suspensions/1/StiffnessSetupMin" ] = { "5.00000" },
[ "bike/data/suspensions/1/CompressionDamping" ] = { "-0.15000" },
[ "bike/data/suspensions/1/CompressionDampingSetupMax" ] = { "-0.15000" },
[ "bike/data/suspensions/1/CompressionDampingSetupMin" ] = { "-0.15000" },
[ "bike/data/suspensions/1/ReboundDamping" ] = { "-0.10000" },
[ "bike/data/suspensions/1/ReboundDampingSetupMin" ] = { "-0.10000" },
[ "bike/data/suspensions/1/ReboundDampingSetupMax" ] = { "-0.10000" },
[ "bike/data/setup/unlock/Suspension" ] = { "-1" }
    }
  },

    frame_shock_absorbers_125_stage_1 = {
    name = [[AMORTİSÖRLER]],
    description = [[Amortisörleri daha gelişmiş bir süspansiyon sistemiyle değiştirerek frenleme ve viraj dengesini artırır.]],
    stage =1,
    staff_level =2,
    category = "frame",
    type =3,
    time =5,
    championship = "c125",
    effects = {
[ "bike/data/DrivingAids/FrontBrakeControl" ] = { "-0.09600" },
[ "bike/data/DrivingAids/FrontBrakeControlSetupMax" ] = { "-0.19200" },
[ "bike/data/DrivingAids/FrontBrakeControlSetupMin" ] = { "0.00000" },
[ "bike/data/lean/rate/TorqueMax" ] = { "-0.00656" },
[ "bike/data/lean/rate/Damping" ] = { "0.08000" },
[ "bike/data/lean/angle/SteerBias" ] = { "0.00000" },
[ "bike/data/NormalLoadEQ/DampingRate" ] = { "-0.28000" },
[ "bike/data/suspensions/0/Stiffness" ] = { "8.00000" },
[ "bike/data/suspensions/0/StiffnessSetupMax" ] = { "8.00000" },
[ "bike/data/suspensions/0/StiffnessSetupMin" ] = { "8.00000" },
[ "bike/data/suspensions/0/CompressionDamping" ] = { "-0.24000" },
[ "bike/data/suspensions/0/CompressionDampingSetupMax" ] = { "-0.40000" },
[ "bike/data/suspensions/0/CompressionDampingSetupMin" ] = { "-0.24000" },
[ "bike/data/suspensions/0/ReboundDamping" ] = { "-0.16000" },
[ "bike/data/suspensions/0/ReboundDampingSetupMax" ] = { "-0.40000" },
[ "bike/data/suspensions/0/ReboundDampingSetupMin" ] = { "-0.16000" },
[ "bike/data/suspensions/1/Stiffness" ] = { "4.00000" },
[ "bike/data/suspensions/1/StiffnessSetupMax" ] = { "4.00000" },
[ "bike/data/suspensions/1/StiffnessSetupMin" ] = { "4.00000" },
[ "bike/data/suspensions/1/CompressionDamping" ] = { "-0.12000" },
[ "bike/data/suspensions/1/CompressionDampingSetupMax" ] = { "-0.12000" },
[ "bike/data/suspensions/1/CompressionDampingSetupMin" ] = { "-0.12000" },
[ "bike/data/suspensions/1/ReboundDamping" ] = { "-0.08000" },
[ "bike/data/suspensions/1/ReboundDampingSetupMin" ] = { "-0.08000" },
[ "bike/data/suspensions/1/ReboundDampingSetupMax" ] = { "-0.08000" },
[ "bike/data/setup/unlock/Suspension" ] = { "-1" }
    }
  },

    frame_shock_absorbers_125_stage_2 = {
    name = [[AMORTİSÖRLER]],
    description = [[Mevcut süspansiyon sistemini daha iyi performans için yeniden ayarlar.]],
    stage =2,
    staff_level =5,
    category = "frame",
    type =3,
    time =10,
    championship = "c125",
    effects = {
[ "bike/data/DrivingAids/FrontBrakeControl" ] = { "-0.05851" },
[ "bike/data/DrivingAids/FrontBrakeControlSetupMax" ] = { "-0.11702" },
[ "bike/data/DrivingAids/FrontBrakeControlSetupMin" ] = { "0.00000" },
[ "bike/data/lean/rate/TorqueMax" ] = { "-0.00400" },
[ "bike/data/lean/rate/Damping" ] = { "0.04876" },
[ "bike/data/lean/angle/SteerBias" ] = { "0.00000" },
[ "bike/data/NormalLoadEQ/DampingRate" ] = { "-0.17066" },
[ "bike/data/suspensions/0/Stiffness" ] = { "4.87600" },
[ "bike/data/suspensions/0/StiffnessSetupMax" ] = { "4.87600" },
[ "bike/data/suspensions/0/StiffnessSetupMin" ] = { "4.87600" },
[ "bike/data/suspensions/0/CompressionDamping" ] = { "-0.14628" },
[ "bike/data/suspensions/0/CompressionDampingSetupMax" ] = { "-0.24380" },
[ "bike/data/suspensions/0/CompressionDampingSetupMin" ] = { "-0.14628" },
[ "bike/data/suspensions/0/ReboundDamping" ] = { "-0.09752" },
[ "bike/data/suspensions/0/ReboundDampingSetupMax" ] = { "-0.24380" },
[ "bike/data/suspensions/0/ReboundDampingSetupMin" ] = { "-0.09752" },
[ "bike/data/suspensions/1/Stiffness" ] = { "2.43800" },
[ "bike/data/suspensions/1/StiffnessSetupMax" ] = { "2.43800" },
[ "bike/data/suspensions/1/StiffnessSetupMin" ] = { "2.43800" },
[ "bike/data/suspensions/1/CompressionDamping" ] = { "-0.07314" },
[ "bike/data/suspensions/1/CompressionDampingSetupMax" ] = { "-0.07314" },
[ "bike/data/suspensions/1/CompressionDampingSetupMin" ] = { "-0.07314" },
[ "bike/data/suspensions/1/ReboundDamping" ] = { "-0.04876" },
[ "bike/data/suspensions/1/ReboundDampingSetupMin" ] = { "-0.04876" },
[ "bike/data/suspensions/1/ReboundDampingSetupMax" ] = { "-0.04876" },
[ "bike/data/setup/unlock/Suspension" ] = { "-1" }
    }
  },

    frame_shock_absorbers_125_stage_3 = {
    name = [[AMORTİSÖRLER]],
    description = [[Bozuk yüzeylerin darbelerini ve sert frenlemenin oluşturduğu sıkışmayı daha etkili sönümleyen gelişmiş amortisörler.]],
    stage =3,
    staff_level =8,
    category = "frame",
    type =3,
    time =15,
    championship = "c125",
    effects = {
[ "bike/data/DrivingAids/FrontBrakeControl" ] = { "0.00000" },
[ "bike/data/DrivingAids/FrontBrakeControlSetupMax" ] = { "0.00000" },
[ "bike/data/DrivingAids/FrontBrakeControlSetupMin" ] = { "0.00000" },
[ "bike/data/lean/rate/TorqueMax" ] = { "0.00000" },
[ "bike/data/lean/rate/Damping" ] = { "0.00000" },
[ "bike/data/lean/angle/SteerBias" ] = { "0.00000" },
[ "bike/data/NormalLoadEQ/DampingRate" ] = { "0.00000" },
[ "bike/data/suspensions/0/Stiffness" ] = { "0.00000" },
[ "bike/data/suspensions/0/StiffnessSetupMax" ] = { "0.00000" },
[ "bike/data/suspensions/0/StiffnessSetupMin" ] = { "0.00000" },
[ "bike/data/suspensions/0/CompressionDamping" ] = { "0.00000" },
[ "bike/data/suspensions/0/CompressionDampingSetupMax" ] = { "0.00000" },
[ "bike/data/suspensions/0/CompressionDampingSetupMin" ] = { "0.00000" },
[ "bike/data/suspensions/0/ReboundDamping" ] = { "0.00000" },
[ "bike/data/suspensions/0/ReboundDampingSetupMax" ] = { "0.00000" },
[ "bike/data/suspensions/0/ReboundDampingSetupMin" ] = { "0.00000" },
[ "bike/data/suspensions/1/Stiffness" ] = { "0.00000" },
[ "bike/data/suspensions/1/StiffnessSetupMax" ] = { "0.00000" },
[ "bike/data/suspensions/1/StiffnessSetupMin" ] = { "0.00000" },
[ "bike/data/suspensions/1/CompressionDamping" ] = { "0.00000" },
[ "bike/data/suspensions/1/CompressionDampingSetupMax" ] = { "0.00000" },
[ "bike/data/suspensions/1/CompressionDampingSetupMin" ] = { "0.00000" },
[ "bike/data/suspensions/1/ReboundDamping" ] = { "0.00000" },
[ "bike/data/suspensions/1/ReboundDampingSetupMin" ] = { "0.00000" },
[ "bike/data/suspensions/1/ReboundDampingSetupMax" ] = { "0.00000" },
[ "bike/data/setup/unlock/Suspension" ] = { "0" }
    }
  },

  -- # GEARBOX - CLUTCH

    gearbox_clutch_125_stage_0 = {
    name = "",
    description = "",
    stage =0,
    staff_level =0,
    category = "gearbox",
    type =0,
    time =0,
    championship = "c125",
    effects = {
[ "bike/data/body/Mass" ] = { "0.00750" },
[ "bike/data/engine/MaxCompressionTorque" ] = { "-0.03000" },
[ "bike/data/gearbox/ClutchingTime" ] = { "0.05000" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00150" }
    }
  },

    gearbox_clutch_125_stage_1 = {
    name = [[DEBRİYAJ]],
    description = [[Standart debriyajı kaydırmalı debriyaj mekanizmasıyla değiştirerek vites küçültürken motosikleti çok daha dengeli hâle getirir.]],
    stage =1,
    staff_level =3,
    category = "gearbox",
    type =0,
    time =5,
    championship = "c125",
    effects = {
[ "bike/data/body/Mass" ] = { "0.00600" },
[ "bike/data/engine/MaxCompressionTorque" ] = { "-0.02400" },
[ "bike/data/gearbox/ClutchingTime" ] = { "0.04000" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00120" }
    }
  },

  gearbox_clutch_125_stage_2 = {
    name = [[DEBRİYAJ]],
    description = [[Motor frenine ve daha hızlı vites değişimlerine daha iyi dayanan, güçlendirilmiş hafif debriyaj plakaları.]],
    stage =2,
    staff_level =6,
    category = "gearbox",
    type =0,
    time =10,
    championship = "c125",
    effects = {
[ "bike/data/body/Mass" ] = { "0.00366" },
[ "bike/data/engine/MaxCompressionTorque" ] = { "-0.01463" },
[ "bike/data/gearbox/ClutchingTime" ] = { "0.02438" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00073" }
    }
  },

    gearbox_clutch_125_stage_3 = {
    name = [[DEBRİYAJ]],
    description = [[Debriyajdaki ısıyı daha etkili dağıtarak daha da hızlı vites değiştirmeyi sağlayan teknoloji.]],
    stage =3,
    staff_level =9,
    category = "gearbox",
    type =0,
    time =15,
    championship = "c125",
    effects = {
[ "bike/data/body/Mass" ] = { "0.00000" },
[ "bike/data/engine/MaxCompressionTorque" ] = { "0.00000" },
[ "bike/data/gearbox/ClutchingTime" ] = { "0.00000" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "0.00000" }
    }
  },

  -- # GEARBOX - RATIOS

    gearbox_ratios_125_stage_0 = {
    name = "",
    description = "",
    stage =0,
    staff_level =0,
    category = "gearbox",
    type =1,
    time =0,
    championship = "c125",
    effects = {
[ "bike/data/engine/MaxTorque" ] = { "-0.00600" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00150" },
[ "bike/data/setup/unlock/Gearbox" ] = { "-1" }
    }
  },

    gearbox_ratios_125_stage_1 = {
    name = [[VİTES ORANLARI]],
    description = [[Daha iyi hızlanma sağlayan küçük bir şanzıman iyileştirmesi.]],
    stage =1,
    staff_level =2,
    category = "gearbox",
    type =1,
    time =10,
    championship = "c125",
    effects = {
[ "bike/data/engine/MaxTorque" ] = { "-0.00480" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00120" },
[ "bike/data/setup/unlock/Gearbox" ] = { "-1" }
    }
  },

   gearbox_ratios_125_stage_2 = {
    name = [[VİTES ORANLARI]],
    description = [[Hızlanmayı daha da artırmak için motosiklete en uygun vites düzeni araştırılır.]],
    stage =2,
    staff_level =6,
    category = "gearbox",
    type =1,
    time =15,
    championship = "c125",
    effects = {
[ "bike/data/engine/MaxTorque" ] = { -0.00293 },
[ "bike/data/gearbox/DifferentialRatio" ] = { -0.00073 },
[ "bike/data/setup/unlock/Gearbox" ] = { -1 }
    }
  },

    gearbox_ratios_125_stage_3 = {
    name = [[VİTES ORANLARI]],
    description = [[Motorun yüksek torkuna ve daha sert vites değişimlerine dayanmak için çeşitli şanzıman parçaları daha hafif ve sağlam malzemelerle değiştirilir.]],
    stage =3,
    staff_level =10,
    category = "gearbox",
    type =1,
    time =20,
    championship = "c125",
    effects = {
[ "bike/data/engine/MaxTorque" ] = { "0.00000" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "0.00000" },
[ "bike/data/setup/unlock/Gearbox" ] = { "0" }
    }
  },

  -- # 250cc ---------------------------------------------------------------------------------------------------------------------

  -- # BRAKES

  brakes_250_stage_0 = {
    name = "",
    description = "",
    stage =0,
    staff_level =0,
    category = "brakes",
    type =0,
    time =0,
    championship = "c250",
    effects = {
[ "bike/data/body/Mass" ] = { "0.01000" },
[ "bike/data/brakes/0/Force" ] = { "-3.00000" },
[ "bike/data/brakes/1/Force" ] = { "-2.00000" },
[ "bike/data/NormalLoadEQ/DampingRate" ] = { "-0.15000" },
[ "bike/data/DrivingAids/FrontBrakeControl" ] = { "-0.35000" },
[ "bike/data/DrivingAids/FrontBrakeControlSetupMax" ] = { "-0.70000" },
[ "bike/data/DrivingAids/FrontBrakeControlSetupMin" ] = { "0.00000" },
[ "bike/data/DrivingAids/RearBrakeControl" ] = { "-0.35000" },
[ "bike/data/DrivingAids/RearBrakeControlSetupMax" ] = { "-0.70000" },
[ "bike/data/DrivingAids/RearBrakeControlSetupMin" ] = { "0.00000" }
    }
  },

  brakes_250_stage_1 = {
    name = [[FRENLER]],
    description = [[En uygun sıcaklığa daha çabuk ulaşan hafif diskler.]],
    stage =1,
    staff_level =12,
    category = "brakes",
    type =0,
    time =25,
    championship = "c250",
    effects = {
[ "bike/data/body/Mass" ] = { "0.00900" },
[ "bike/data/brakes/0/Force" ] = { "-2.70000" },
[ "bike/data/brakes/1/Force" ] = { "-1.80000" },
[ "bike/data/NormalLoadEQ/DampingRate" ] = { "-0.13500" },
[ "bike/data/DrivingAids/FrontBrakeControl" ] = { "-0.31500" },
[ "bike/data/DrivingAids/FrontBrakeControlSetupMax" ] = { "-0.63000" },
[ "bike/data/DrivingAids/FrontBrakeControlSetupMin" ] = { "0.00000" },
[ "bike/data/DrivingAids/RearBrakeControl" ] = { "-0.31500" },
[ "bike/data/DrivingAids/RearBrakeControlSetupMax" ] = { "-0.63000" },
[ "bike/data/DrivingAids/RearBrakeControlSetupMin" ] = { "0.00000" }
    }
  },

  brakes_250_stage_2 = {
    name = [[FRENLER]],
    description = [[Daha yüksek dayanıklılık ve frenleme gücü.]],
    stage =2,
    staff_level =13,
    category = "brakes",
    type =0,
    time =20,
    championship = "c250",
    effects = {
[ "bike/data/body/Mass" ] = { "0.00734" },
[ "bike/data/brakes/0/Force" ] = { "-2.20170" },
[ "bike/data/brakes/1/Force" ] = { "-1.46780" },
[ "bike/data/NormalLoadEQ/DampingRate" ] = { "-0.11009" },
[ "bike/data/DrivingAids/FrontBrakeControl" ] = { "-0.25687" },
[ "bike/data/DrivingAids/FrontBrakeControlSetupMax" ] = { "-0.51373" },
[ "bike/data/DrivingAids/FrontBrakeControlSetupMin" ] = { "0.00000" },
[ "bike/data/DrivingAids/RearBrakeControl" ] = { "-0.25687" },
[ "bike/data/DrivingAids/RearBrakeControlSetupMax" ] = { "-0.51373" },
[ "bike/data/DrivingAids/RearBrakeControlSetupMin" ] = { "0.00000" }
    }
  },

    brakes_250_stage_3 = {
    name = [[FRENLER]],
    description = [[Fren performansında ek iyileştirme.]],
    stage =3,
    staff_level =14,
    category = "brakes",
    type =0,
    time =25,
    championship = "c250",
    effects = {
[ "bike/data/body/Mass" ] = { "0.00458" },
[ "bike/data/brakes/0/Force" ] = { "-1.37402" },
[ "bike/data/brakes/1/Force" ] = { "-0.91602" },
[ "bike/data/NormalLoadEQ/DampingRate" ] = { "-0.06870" },
[ "bike/data/DrivingAids/FrontBrakeControl" ] = { "-0.16030" },
[ "bike/data/DrivingAids/FrontBrakeControlSetupMax" ] = { "-0.32061" },
[ "bike/data/DrivingAids/FrontBrakeControlSetupMin" ] = { "0.00000" },
[ "bike/data/DrivingAids/RearBrakeControl" ] = { "-0.16030" },
[ "bike/data/DrivingAids/RearBrakeControlSetupMax" ] = { "-0.32061" },
[ "bike/data/DrivingAids/RearBrakeControlSetupMin" ] = { "0.00000" }
    }
  },

    brakes_250_stage_4 = {
    name = [[FRENLER]],
    description = [[Son derece hafif disk frenler.]],
    stage =4,
    staff_level =15,
    category = "brakes",
    type =0,
    time =30,
    championship = "c250",
    effects = {
[ "bike/data/body/Mass" ] = { "0.00000" },
[ "bike/data/brakes/0/Force" ] = { "0.00000" },
[ "bike/data/brakes/1/Force" ] = { "0.00000" },
[ "bike/data/NormalLoadEQ/DampingRate" ] = { "0.00000" },
[ "bike/data/DrivingAids/FrontBrakeControl" ] = { "0.00000" },
[ "bike/data/DrivingAids/FrontBrakeControlSetupMax" ] = { "0.00000" },
[ "bike/data/DrivingAids/FrontBrakeControlSetupMin" ] = { "0.00000" },
[ "bike/data/DrivingAids/RearBrakeControl" ] = { "0.00000" },
[ "bike/data/DrivingAids/RearBrakeControlSetupMax" ] = { "0.00000" },
[ "bike/data/DrivingAids/RearBrakeControlSetupMin" ] = { "0.00000" }
    }
  },

  -- # Soft tyres

    tyres_soft_compound_250_stage_0 = {
    name = "",
    description = "",
    stage =0,
    staff_level =0,
    category = "brakes",
    type =1,
    time =0,
    championship = "c250",
    effects = {
[ "bike/data/DrivingAids/TractionControl" ] = { "-0.25000" },
[ "bike/data/DrivingAids/TractionControlSetupMax" ] = { "-0.50000" },
[ "bike/data/DrivingAids/TractionControlSetupMin" ] = { "0.00000" },
[ "bike/data/wheels/0/Soft/LongitudinalFrictionCoeffMaxCoeff " ] = { "-0.10000" },
[ "bike/data/wheels/0/Soft/LateralFrictionMaxCoeff" ] = { "-0.10000" },
[ "bike/data/wheels/0/Soft/LateralFrictionCoeffLimitMaxCoeff" ] = { "-0.10000" },
[ "bike/data/wheels/1/Soft/LongitudinalFrictionCoeffMaxCoeff " ] = { "-0.10000" },
[ "bike/data/wheels/1/Soft/LateralFrictionMaxCoeff" ] = { "-0.10000" },
[ "bike/data/wheels/1/Soft/LateralFrictionCoeffLimitMaxCoeff" ] = { "-0.10000" }
    }
  },

    tyres_soft_compound_250_stage_1 = {
    name = [[YUMUŞAK HAMUR]],
    description = [[Yumuşak lastik hamurunun dayanıklılığını ve yol tutuşunu biraz iyileştirir.]],
    stage =1,
    staff_level =11,
    category = "brakes",
    type =1,
    time =20,
    championship = "c250",
    effects = {
[ "bike/data/DrivingAids/TractionControl" ] = { "-0.22500" },
[ "bike/data/DrivingAids/TractionControlSetupMax" ] = { "-0.45000" },
[ "bike/data/DrivingAids/TractionControlSetupMin" ] = { "0.00000" },
[ "bike/data/wheels/0/Soft/LongitudinalFrictionCoeffMaxCoeff " ] = { "-0.09000" },
[ "bike/data/wheels/0/Soft/LateralFrictionMaxCoeff" ] = { "-0.09000" },
[ "bike/data/wheels/0/Soft/LateralFrictionCoeffLimitMaxCoeff" ] = { "-0.09000" },
[ "bike/data/wheels/1/Soft/LongitudinalFrictionCoeffMaxCoeff " ] = { "-0.09000" },
[ "bike/data/wheels/1/Soft/LateralFrictionMaxCoeff" ] = { "-0.09000" },
[ "bike/data/wheels/1/Soft/LateralFrictionCoeffLimitMaxCoeff" ] = { "-0.09000" }
    }
  },

    tyres_soft_compound_250_stage_2 = {
    name = [[YUMUŞAK HAMUR]],
    description = [[Lastiklerin performansını daha uzun süre korumasını ve daha dayanıklı olmasını sağlayan yeni yumuşak hamur.]],
    stage =2,
    staff_level =12,
    category = "brakes",
    type =1,
    time =25,
    championship = "c250",
    effects = {
[ "bike/data/DrivingAids/TractionControl" ] = { "-0.19119" },
[ "bike/data/DrivingAids/TractionControlSetupMax" ] = { "-0.38238" },
[ "bike/data/DrivingAids/TractionControlSetupMin" ] = { "0.00000" },
[ "bike/data/wheels/0/Soft/LongitudinalFrictionCoeffMaxCoeff " ] = { "-0.07648" },
[ "bike/data/wheels/0/Soft/LateralFrictionMaxCoeff" ] = { "-0.07648" },
[ "bike/data/wheels/0/Soft/LateralFrictionCoeffLimitMaxCoeff" ] = { "-0.07648" },
[ "bike/data/wheels/1/Soft/LongitudinalFrictionCoeffMaxCoeff " ] = { "-0.07648" },
[ "bike/data/wheels/1/Soft/LateralFrictionMaxCoeff" ] = { "-0.07648" },
[ "bike/data/wheels/1/Soft/LateralFrictionCoeffLimitMaxCoeff" ] = { "-0.07648" }
    }
  },

    tyres_soft_compound_250_stage_3 = {
    name = [[YUMUŞAK HAMUR]],
    description = [[Yumuşak lastik hamurunun yol tutuşunda önemli iyileştirme.]],
    stage =3,
    staff_level =13,
    category = "brakes",
    type =1,
    time =30,
    championship = "c250",
    effects = {
[ "bike/data/DrivingAids/TractionControl" ] = { "-0.14546" },
[ "bike/data/DrivingAids/TractionControlSetupMax" ] = { "-0.29091" },
[ "bike/data/DrivingAids/TractionControlSetupMin" ] = { "0.00000" },
[ "bike/data/wheels/0/Soft/LongitudinalFrictionCoeffMaxCoeff " ] = { "-0.05818" },
[ "bike/data/wheels/0/Soft/LateralFrictionMaxCoeff" ] = { "-0.05818" },
[ "bike/data/wheels/0/Soft/LateralFrictionCoeffLimitMaxCoeff" ] = { "-0.05818" },
[ "bike/data/wheels/1/Soft/LongitudinalFrictionCoeffMaxCoeff " ] = { "-0.05818" },
[ "bike/data/wheels/1/Soft/LateralFrictionMaxCoeff" ] = { "-0.05818" },
[ "bike/data/wheels/1/Soft/LateralFrictionCoeffLimitMaxCoeff" ] = { "-0.05818" }
    }
  },

    tyres_soft_compound_250_stage_4 = {
    name = [[YUMUŞAK HAMUR]],
    description = [[Yumuşak lastik hamurunun dayanıklılığını artırır.]],
    stage =4,
    staff_level =14,
    category = "brakes",
    type =1,
    time =35,
    championship = "c250",
    effects = {
[ "bike/data/DrivingAids/TractionControl" ] = { "-0.08360" },
[ "bike/data/DrivingAids/TractionControlSetupMax" ] = { "-0.16721" },
[ "bike/data/DrivingAids/TractionControlSetupMin" ] = { "0.00000" },
[ "bike/data/wheels/0/Soft/LongitudinalFrictionCoeffMaxCoeff " ] = { "-0.03344" },
[ "bike/data/wheels/0/Soft/LateralFrictionMaxCoeff" ] = { "-0.03344" },
[ "bike/data/wheels/0/Soft/LateralFrictionCoeffLimitMaxCoeff" ] = { "-0.03344" },
[ "bike/data/wheels/1/Soft/LongitudinalFrictionCoeffMaxCoeff " ] = { "-0.03344" },
[ "bike/data/wheels/1/Soft/LateralFrictionMaxCoeff" ] = { "-0.03344" },
[ "bike/data/wheels/1/Soft/LateralFrictionCoeffLimitMaxCoeff" ] = { "-0.03344" }
    }
  },

    tyres_soft_compound_250_stage_5 = {
    name = [[YUMUŞAK HAMUR]],
    description = [[Lastik teknolojisinin sınırlarını zorlar. En iyi yumuşak lastik hamuru.]],
    stage =5,
    staff_level =15,
    category = "brakes",
    type =1,
    time =40,
    championship = "c250",
    effects = {
[ "bike/data/DrivingAids/TractionControl" ] = { "0.00000" },
[ "bike/data/DrivingAids/TractionControlSetupMax" ] = { "0.00000" },
[ "bike/data/DrivingAids/TractionControlSetupMin" ] = { "0.00000" },
[ "bike/data/wheels/0/Soft/LongitudinalFrictionCoeffMaxCoeff " ] = { "0.00000" },
[ "bike/data/wheels/0/Soft/LateralFrictionMaxCoeff" ] = { "0.00000" },
[ "bike/data/wheels/0/Soft/LateralFrictionCoeffLimitMaxCoeff" ] = { "0.00000" },
[ "bike/data/wheels/1/Soft/LongitudinalFrictionCoeffMaxCoeff " ] = { "0.00000" },
[ "bike/data/wheels/1/Soft/LateralFrictionMaxCoeff" ] = { "0.00000" },
[ "bike/data/wheels/1/Soft/LateralFrictionCoeffLimitMaxCoeff" ] = { "0.00000" }
    }
  },

    -- # Hard tyres

    tyres_hard_compound_250_stage_0 = {
    name = "",
    description = "",
    stage =0,
    staff_level =0,
    category = "brakes",
    type =2,
    time =0,
    championship = "c250",
    effects = {
[ "bike/data/DrivingAids/TractionControl" ] = { "-0.25000" },
[ "bike/data/DrivingAids/TractionControlSetupMax" ] = { "-0.50000" },
[ "bike/data/DrivingAids/TractionControlSetupMin" ] = { "0.00000" },
[ "bike/data/wheels/0/Hard/LongitudinalFrictionCoeffMaxCoeff " ] = { "-0.10000" },
[ "bike/data/wheels/0/Hard/LateralFrictionMaxCoeff" ] = { "-0.10000" },
[ "bike/data/wheels/0/Hard/LateralFrictionCoeffLimitMaxCoeff" ] = { "-0.10000" },
[ "bike/data/wheels/1/Hard/LongitudinalFrictionCoeffMaxCoeff " ] = { "-0.10000" },
[ "bike/data/wheels/1/Hard/LateralFrictionMaxCoeff" ] = { "-0.10000" },
[ "bike/data/wheels/1/Hard/LateralFrictionCoeffLimitMaxCoeff" ] = { "-0.10000" }
    }
  },

    tyres_hard_compound_250_stage_1 = {
    name = [[SERT HAMUR]],
    description = [[Sert lastik hamurunun dayanıklılığını ve yol tutuşunu biraz iyileştirir.]],
    stage =1,
    staff_level =11,
    category = "brakes",
    type =2,
    time =20,
    championship = "c250",
    effects = {
[ "bike/data/DrivingAids/TractionControl" ] = { "-0.22500" },
[ "bike/data/DrivingAids/TractionControlSetupMax" ] = { "-0.45000" },
[ "bike/data/DrivingAids/TractionControlSetupMin" ] = { "0.00000" },
[ "bike/data/wheels/0/Hard/LongitudinalFrictionCoeffMaxCoeff " ] = { "-0.09000" },
[ "bike/data/wheels/0/Hard/LateralFrictionMaxCoeff" ] = { "-0.09000" },
[ "bike/data/wheels/0/Hard/LateralFrictionCoeffLimitMaxCoeff" ] = { "-0.09000" },
[ "bike/data/wheels/1/Hard/LongitudinalFrictionCoeffMaxCoeff " ] = { "-0.09000" },
[ "bike/data/wheels/1/Hard/LateralFrictionMaxCoeff" ] = { "-0.09000" },
[ "bike/data/wheels/1/Hard/LateralFrictionCoeffLimitMaxCoeff" ] = { "-0.09000" }
    }
  },

    tyres_hard_compound_250_stage_2 = {
    name = [[SERT HAMUR]],
    description = [[Lastiklerin performansını daha uzun süre korumasını ve daha dayanıklı olmasını sağlayan yeni sert hamur.]],
    stage =2,
    staff_level =12,
    category = "brakes",
    type =2,
    time =25,
    championship = "c250",
    effects = {
[ "bike/data/DrivingAids/TractionControl" ] = { "-0.19119" },
[ "bike/data/DrivingAids/TractionControlSetupMax" ] = { "-0.38238" },
[ "bike/data/DrivingAids/TractionControlSetupMin" ] = { "0.00000" },
[ "bike/data/wheels/0/Hard/LongitudinalFrictionCoeffMaxCoeff " ] = { "-0.07648" },
[ "bike/data/wheels/0/Hard/LateralFrictionMaxCoeff" ] = { "-0.07648" },
[ "bike/data/wheels/0/Hard/LateralFrictionCoeffLimitMaxCoeff" ] = { "-0.07648" },
[ "bike/data/wheels/1/Hard/LongitudinalFrictionCoeffMaxCoeff " ] = { "-0.07648" },
[ "bike/data/wheels/1/Hard/LateralFrictionMaxCoeff" ] = { "-0.07648" },
[ "bike/data/wheels/1/Hard/LateralFrictionCoeffLimitMaxCoeff" ] = { "-0.07648" }
    }
  },

    tyres_hard_compound_250_stage_3 = {
    name = [[SERT HAMUR]],
    description = [[Sert lastik hamurunun yol tutuşunda önemli iyileştirme.]],
    stage =3,
    staff_level =13,
    category = "brakes",
    type =2,
    time =30,
    championship = "c250",
    effects = {
[ "bike/data/DrivingAids/TractionControl" ] = { "-0.14546" },
[ "bike/data/DrivingAids/TractionControlSetupMax" ] = { "-0.29091" },
[ "bike/data/DrivingAids/TractionControlSetupMin" ] = { "0.00000" },
[ "bike/data/wheels/0/Hard/LongitudinalFrictionCoeffMaxCoeff " ] = { "-0.05818" },
[ "bike/data/wheels/0/Hard/LateralFrictionMaxCoeff" ] = { "-0.05818" },
[ "bike/data/wheels/0/Hard/LateralFrictionCoeffLimitMaxCoeff" ] = { "-0.05818" },
[ "bike/data/wheels/1/Hard/LongitudinalFrictionCoeffMaxCoeff " ] = { "-0.05818" },
[ "bike/data/wheels/1/Hard/LateralFrictionMaxCoeff" ] = { "-0.05818" },
[ "bike/data/wheels/1/Hard/LateralFrictionCoeffLimitMaxCoeff" ] = { "-0.05818" }
    }
  },

tyres_hard_compound_250_stage_4 = {
    name = [[SERT HAMUR]],
    description = [[Sert lastik hamurunun dayanıklılığını artırır.]],
    stage =4,
    staff_level =14,
    category = "brakes",
    type =2,
    time =35,
    championship = "c250",
    effects = {
[ "bike/data/DrivingAids/TractionControl" ] = { "-0.08360" },
[ "bike/data/DrivingAids/TractionControlSetupMax" ] = { "-0.16721" },
[ "bike/data/DrivingAids/TractionControlSetupMin" ] = { "0.00000" },
[ "bike/data/wheels/0/Hard/LongitudinalFrictionCoeffMaxCoeff " ] = { "-0.03344" },
[ "bike/data/wheels/0/Hard/LateralFrictionMaxCoeff" ] = { "-0.03344" },
[ "bike/data/wheels/0/Hard/LateralFrictionCoeffLimitMaxCoeff" ] = { "-0.03344" },
[ "bike/data/wheels/1/Hard/LongitudinalFrictionCoeffMaxCoeff " ] = { "-0.03344" },
[ "bike/data/wheels/1/Hard/LateralFrictionMaxCoeff" ] = { "-0.03344" },
[ "bike/data/wheels/1/Hard/LateralFrictionCoeffLimitMaxCoeff" ] = { "-0.03344" }
    }
  },

    tyres_hard_compound_250_stage_5 = {
    name = [[SERT HAMUR]],
    description = [[Lastik teknolojisinin sınırlarını zorlar. En iyi sert lastik hamuru.]],
    stage =5,
    staff_level =15,
    category = "brakes",
    type =2,
    time =40,
    championship = "c250",
    effects = {
[ "bike/data/DrivingAids/TractionControl" ] = { "0.00000" },
[ "bike/data/DrivingAids/TractionControlSetupMax" ] = { "0.00000" },
[ "bike/data/DrivingAids/TractionControlSetupMin" ] = { "0.00000" },
[ "bike/data/wheels/0/Hard/LongitudinalFrictionCoeffMaxCoeff " ] = { "0.00000" },
[ "bike/data/wheels/0/Hard/LateralFrictionMaxCoeff" ] = { "0.00000" },
[ "bike/data/wheels/0/Hard/LateralFrictionCoeffLimitMaxCoeff" ] = { "0.00000" },
[ "bike/data/wheels/1/Hard/LongitudinalFrictionCoeffMaxCoeff " ] = { "0.00000" },
[ "bike/data/wheels/1/Hard/LateralFrictionMaxCoeff" ] = { "0.00000" },
[ "bike/data/wheels/1/Hard/LateralFrictionCoeffLimitMaxCoeff" ] = { "0.00000" }
    }
  },

  -- # ENGINE

  -- # ENGINE - BALANCING

    engine_balancing_250_stage_0 = {
    name = "",
    description = "",
    stage =0,
    staff_level =0,
    category = "engine",
    type =0,
    time =0,
    championship = "c250",
    effects = {
[ "bike/data/engine/MaxTorque" ] = { "-0.01600" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00320" }
    }
  },

    engine_balancing_250_stage_1 = {
    name = [[BALANS AYARI]],
    description = [[Motorun hassas biçimde dengelenmesi; aşırı ses, titreşim ve sürtünmeden kaynaklanan enerji kaybını azaltarak verimliliği artırır.]],
    stage =1,
    staff_level =14,
    category = "engine",
    type =0,
    time =20,
    championship = "c250",
    effects = {
[ "bike/data/engine/MaxTorque" ] = { "-0.00800" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00160" }
    }
  },

    engine_balancing_250_stage_2 = {
    name = [[BALANS AYARI]],
    description = [[Tüm devir aralığında daha düzenli bir güç dağılımı sağlayarak motorun performansını biraz artırır.]],
    stage =2,
    staff_level =17,
    category = "engine",
    type =0,
    time =20,
    championship = "c250",
    effects = {
[ "bike/data/engine/MaxTorque" ] = { "0.00000" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "0.00000" }
    }
  },

  -- # ENGINE - RADIATOR

    engine_radiator_250_stage_0 = {
    name = "",
    description = "",
    stage =0,
    staff_level =0,
    category = "engine",
    type =1,
    time =0,
    championship = "c250",
    effects = {
[ "bike/data/aerodynamic/BikeDragCoeff" ] = { "0.00675" },
[ "bike/data/aerodynamic/BikeReferenceArea" ] = { "0.00675" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00320" },
[ "bike/data/engine/MaxTorque" ] = { "-0.01600" }
    }
  },

    engine_radiator_250_stage_1 = {
    name = [[RADYATÖR]],
    description = [[Radyatörün hava girişleri iyileştirilerek hava direnci azaltılır ve havalandırma artırılır.]],
    stage =1,
    staff_level =12,
    category = "engine",
    type =1,
    time =20,
    championship = "c250",
    effects = {
[ "bike/data/aerodynamic/BikeDragCoeff" ] = { "0.00338" },
[ "bike/data/aerodynamic/BikeReferenceArea" ] = { "0.00338" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00160" },
[ "bike/data/engine/MaxTorque" ] = { "-0.00800" }
    }
  },

    engine_radiator_250_stage_2 = {
    name = [[RADYATÖR]],
    description = [[Radyatörün aerodinamiğini ve motoru soğutma performansını daha da iyileştirir.]],
    stage =2,
    staff_level =16,
    category = "engine",
    type =1,
    time =30,
    championship = "c250",
    effects = {
[ "bike/data/aerodynamic/BikeDragCoeff" ] = { "0.00000" },
[ "bike/data/aerodynamic/BikeReferenceArea" ] = { "0.00000" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "0.00000" },
[ "bike/data/engine/MaxTorque" ] = { "0.00000" }
    }
  },

  -- # ENGINE - CRANKSHAFT

    engine_crankshaft_250_stage_0 = {
    name = "",
    description = "",
    stage =0,
    staff_level =0,
    category = "engine",
    type =2,
    time =0,
    championship = "c250",
    effects = {
[ "bike/data/engine/MaxTorque" ] = { "0.00800" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00320" },
[ "bike/data/lean/rate/TorqueMax" ] = { "-0.03000" },
[ "bike/data/lean/rate/Damping" ] = { "0.03000" },
[ "bike/data/lean/angle/SteerBias" ] = { "0.00000" }
    }
  },

    engine_crankshaft_250_stage_1 = {
    name = [[KRANK MİLİ]],
    description = [[Motosiklet üzerindeki jiroskopik kuvvetleri dengelemek için tekerleklerin ters yönünde dönen bir krank mili sistemi geliştirir. Bir miktar motor gücünden ödün vererek motosikletin viraj tepkisini iyileştirir.]],
    stage =1,
    staff_level =20,
    category = "engine",
    type =2,
    time =40,
    championship = "c250",
    effects = {
[ "bike/data/engine/MaxTorque" ] = { "0.00000" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "0.00000" },
[ "bike/data/lean/rate/TorqueMax" ] = { "0.00000" },
[ "bike/data/lean/rate/Damping" ] = { "0.00000" },
[ "bike/data/lean/angle/SteerBias" ] = { "0.00000" }
    }
  },


  -- # ENGINE - PORT POLISH

    engine_portpolish_250_stage_0 = {
    name = "",
    description = "",
    stage =0,
    staff_level =0,
    category = "engine",
    type =3,
    time =0,
    championship = "c250",
    effects = {
[ "bike/data/engine/MaxTorque" ] = { "-0.00800" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00200" }
    }
  },

    engine_portpolish_250_stage_1 = {
    name = [[KANAL PARLATMA]],
    description = [[Motorun emme kanalları parlatılarak hava akışı düzgünleştirilir.]],
    stage =1,
    staff_level =9,
    category = "engine",
    type =3,
    time =10,
    championship = "c250",
    effects = {
[ "bike/data/engine/MaxTorque" ] = { "-0.00400" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00100" }
    }
  },

    engine_portpolish_250_stage_2 = {
    name = [[KANAL PARLATMA]],
    description = [[En yüksek performans için egzoz kanalları da parlatılır.]],
    stage =2,
    staff_level =13,
    category = "engine",
    type =3,
    time =15,
    championship = "c250",
    effects = {
[ "bike/data/engine/MaxTorque" ] = { "0.00000" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "0.00000" }
    }
  },

  -- # ENGINE - VALVES

    engine_valves_250_stage_0 = {
    name = "",
    description = "",
    stage =0,
    staff_level =0,
    category = "engine",
    type =4,
    time =0,
    championship = "c250",
    effects = {
[ "bike/data/engine/MaxTorque" ] = { "-0.04000" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00720" },
[ "bike/data/DrivingAids/TractionControl" ] = { "-0.25000" },
[ "bike/data/DrivingAids/TractionControlSetupMax" ] = { "-0.50000" },
[ "bike/data/DrivingAids/TractionControlSetupMin" ] = { "0.00000" }
    }
  },

    engine_valves_250_stage_1 = {
    name = [[SUPAPLAR]],
    description = [[Daha güvenilir yaylı supaplar geliştirir.]],
    stage =1,
    staff_level =11,
    category = "engine",
    type =4,
    time =20,
    championship = "c250",
    effects = {
[ "bike/data/engine/MaxTorque" ] = { "-0.03600" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00648" },
[ "bike/data/DrivingAids/TractionControl" ] = { "-0.22500" },
[ "bike/data/DrivingAids/TractionControlSetupMax" ] = { "-0.45000" },
[ "bike/data/DrivingAids/TractionControlSetupMin" ] = { "0.00000" }
    }
  },

    engine_valves_250_stage_2 = {
    name = [[SUPAPLAR]],
    description = [[Gelişmiş silindir kapağı teknolojisi. Yayların yerini külbütör mekanizmaları alır. Motor performansı artar.]],
    stage =2,
    staff_level =14,
    category = "engine",
    type =4,
    time =30,
    championship = "c250",
    effects = {
[ "bike/data/engine/MaxTorque" ] = { "-0.02936" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00528" },
[ "bike/data/DrivingAids/TractionControl" ] = { "-0.18348" },
[ "bike/data/DrivingAids/TractionControlSetupMax" ] = { "-0.36695" },
[ "bike/data/DrivingAids/TractionControlSetupMin" ] = { "0.00000" }
    }
  },

    engine_valves_250_stage_3 = {
    name = [[SUPAPLAR]],
    description = [[Supapların açılma ve kapanma hızı artırılarak motor gücü yükseltilir.]],
    stage =3,
    staff_level =17,
    category = "engine",
    type =4,
    time =40,
    championship = "c250",
    effects = {
[ "bike/data/engine/MaxTorque" ] = { "-0.01832" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00330" },
[ "bike/data/DrivingAids/TractionControl" ] = { "-0.11450" },
[ "bike/data/DrivingAids/TractionControlSetupMax" ] = { "-0.22900" },
[ "bike/data/DrivingAids/TractionControlSetupMin" ] = { "0.00000" }
    }
  },

    engine_valves_250_stage_4 = {
    name = [[SUPAPLAR]],
    description = [[Külbütör mekanizmaları pnömatik supaplarla değiştirilir. Performans ve güvenilirlik daha da artar.]],
    stage =4,
    staff_level =19,
    category = "engine",
    type =4,
    time =50,
    championship = "c250",
    effects = {
[ "bike/data/engine/MaxTorque" ] = { "0.00000" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "0.00000" },
[ "bike/data/DrivingAids/TractionControl" ] = { "0.00000" },
[ "bike/data/DrivingAids/TractionControlSetupMax" ] = { "0.00000" },
[ "bike/data/DrivingAids/TractionControlSetupMin" ] = { "0.00000" }
    }
  },

  -- # EXHAUST - MANIFOLD

    exhaust_manifold_250_stage_0 = {
    name = "",
    description = "",
    stage =0,
    staff_level =0,
    category = "exhaust",
    type =0,
    time =0,
    championship = "c250",
    effects = {
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00600" },
[ "bike/data/engine/MaxTorque" ] = { "-0.02560" },
[ "bike/data/body/Mass" ] = { "0.02000" }
    }
  },

    exhaust_manifold_250_stage_1 = {
    name = [[MANİFOLD]],
    description = [[Manifold, daha sağlam ve hafif malzemelerden yapılmış bir modelle değiştirilir.]],
    stage =1,
    staff_level =11,
    category = "exhaust",
    type =0,
    time =20,
    championship = "c250",
    effects = {
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00480" },
[ "bike/data/engine/MaxTorque" ] = { "-0.02048" },
[ "bike/data/body/Mass" ] = { "0.01600" }
    }
  },

    exhaust_manifold_250_stage_2 = {
    name = [[MANİFOLD]],
    description = [[İkinci aşama geliştirmesi, daha verimli hava akışı için manifoldu motorun ateşleme sırasına göre ayarlar.]],
    stage =2,
    staff_level =14,
    category = "exhaust",
    type =0,
    time =25,
    championship = "c250",
    effects = {
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00293" },
[ "bike/data/engine/MaxTorque" ] = { "-0.01248" },
[ "bike/data/body/Mass" ] = { "0.00975" }
    }
  },

   exhaust_manifold_250_stage_3 = {
    name = [[MANİFOLD]],
    description = [[Bu geliştirme, hava akışındaki basınç değişimlerinden yararlanarak gazları motorun içinden tam zamanında geçirmek için manifoldu daha da iyileştirir.]],
    stage =3,
    staff_level =17,
    category = "exhaust",
    type =0,
    time =30,
    championship = "c250",
    effects = {
[ "bike/data/gearbox/DifferentialRatio" ] = { "0.00000" },
[ "bike/data/engine/MaxTorque" ] = { "0.00000" },
[ "bike/data/body/Mass" ] = { "0.00000" }
    }
  },

  -- # EXHAUST - EXHAUST

    exhaust_exhaust_250_stage_0 = {
    name = "",
    description = "",
    stage =0,
    staff_level =0,
    category = "exhaust",
    type =1,
    time =0,
    championship = "c250",
    effects = {
[ "bike/data/body/Mass" ] = { "0.02000" },
[ "bike/data/engine/MaxTorque" ] = { "-0.00800" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00160" },
[ "bike/data/lean/rate/TorqueMax" ] = { "-0.00500" },
[ "bike/data/lean/rate/Damping" ] = { "0.03000" }
    }
  },

    exhaust_exhaust_250_stage_1 = {
    name = [[EGZOZ]],
    description = [[Motosiklete daha hafif ve sağlam yeni bir egzoz sistemi takılır.]],
    stage =1,
    staff_level =12,
    category = "exhaust",
    type =1,
    time =20,
    championship = "c250",
    effects = {
[ "bike/data/body/Mass" ] = { "0.01000" },
[ "bike/data/engine/MaxTorque" ] = { "-0.00400" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00080" },
[ "bike/data/lean/rate/TorqueMax" ] = { "-0.00250" },
[ "bike/data/lean/rate/Damping" ] = { "0.01501" }
    }
  },

    exhaust_exhaust_250_stage_2 = {
    name = [[EGZOZ]],
    description = [[Egzozun hava akışında ve dayanıklılığında küçük iyileştirmeler.]],
    stage =2,
    staff_level =15,
    category = "exhaust",
    type =1,
    time =30,
    championship = "c250",
    effects = {
[ "bike/data/body/Mass" ] = { "0.00000" },
[ "bike/data/engine/MaxTorque" ] = { "0.00000" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "0.00000" },
[ "bike/data/lean/rate/TorqueMax" ] = { "0.00000" },
[ "bike/data/lean/rate/Damping" ] = { "0.00000" }
    }
  },

  -- # EXHAUST - AIR

    exhaust_air_250_stage_0 = {
    name = "",
    description = "",
    stage =0,
    staff_level =0,
    category = "exhaust",
    type =2,
    time =0,
    championship = "c250",
    effects = {
[ "bike/data/engine/MaxTorque" ] = { "-0.01600" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00400" }
    }
  },

    exhaust_air_250_stage_1 = {
    name = [[HAVA FİLTRESİ]],
    description = [[Hava filtresi, kirin motora ulaşmasını engelleyen daha dayanıklı ve verimli bir modelle değiştirilir.]],
    stage =1,
    staff_level =13,
    category = "exhaust",
    type =2,
    time =25,
    championship = "c250",
    effects = {
[ "bike/data/engine/MaxTorque" ] = { "0.00000" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "0.00000" }
    }
  },


  -- # EXHAUST - OIL / FUEL

    exhaust_oilfuel_250_stage_0 = {
    name = "",
    description = "",
    stage =0,
    staff_level =0,
    category = "exhaust",
    type =3,
    time =0,
    championship = "c250",
    effects = {
[ "bike/data/engine/MaxTorque" ] = { "-0.02240" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00560" }
    }
  },

    exhaust_oilfuel_250_stage_1 = {
    name = [[YAĞ/YAKIT]],
    description = [[Motorda kullanılan yağın kalitesini yükselterek sürtünmeyi azaltır.]],
    stage =1,
    staff_level =10,
    category = "exhaust",
    type =3,
    time =15,
    championship = "c250",
    effects = {
[ "bike/data/engine/MaxTorque" ] = { "-0.01120" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00280" }
    }
  },

    exhaust_oilfuel_250_stage_2 = {
    name = [[YAĞ/YAKIT]],
    description = [[Daha fazla enerji açığa çıkaran ve daha yüksek güç sağlayan, yüksek kaliteli ve oktanı artırılmış yakıt geliştirir.]],
    stage =2,
    staff_level =12,
    category = "exhaust",
    type =3,
    time =20,
    championship = "c250",
    effects = {
[ "bike/data/engine/MaxTorque" ] = { "0.00000" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "0.00000" }
    }
  },

  -- # FRAME - CHASSIS

    frame_chassis_250_stage_0 = {
    name = "",
    description = "",
    stage =0,
    staff_level =0,
    category = "frame",
    type =0,
    time =0,
    championship = "c250",
    effects = {
[ "bike/data/body/Mass" ] = { "0.04000" },
[ "bike/data/lean/rate/TorqueMax" ] = { "-0.04000" },
[ "bike/data/lean/rate/Damping" ] = { "0.12000" },
[ "bike/data/lean/angle/SteerBias" ] = { "0.00000" }
    }
  },

    frame_chassis_250_stage_1 = {
    name = [[ŞASİ]],
    description = [[Şasi yapımı için hem sağlam hem hafif yeni malzemeler geliştirir.]],
    stage =1,
    staff_level =12,
    category = "frame",
    type =0,
    time =20,
    championship = "c250",
    effects = {
[ "bike/data/body/Mass" ] = { "0.03200" },
[ "bike/data/lean/rate/TorqueMax" ] = { "-0.03200" },
[ "bike/data/lean/rate/Damping" ] = { "0.09600" },
[ "bike/data/lean/angle/SteerBias" ] = { "0.00000" }
    }
  },

    frame_chassis_250_stage_2 = {
    name = [[ŞASİ]],
    description = [[Ağırlık daha da azaltılarak hızlanma ve son hız önemli ölçüde iyileştirilir.]],
    stage =2,
    staff_level =15,
    category = "frame",
    type =0,
    time =30,
    championship = "c250",
    effects = {
[ "bike/data/body/Mass" ] = { "0.01950" },
[ "bike/data/lean/rate/TorqueMax" ] = { "-0.01950" },
[ "bike/data/lean/rate/Damping" ] = { "0.05851" },
[ "bike/data/lean/angle/SteerBias" ] = { "0.00000" }
    }
  },

    frame_chassis_250_stage_3 = {
    name = [[ŞASİ]],
    description = [[Ağırlık sınırlarını aşmadan şasiyi güçlendirerek virajlarda daha yüksek denge sağlar.]],
    stage =3,
    staff_level =18,
    category = "frame",
    type =0,
    time =40,
    championship = "c250",
    effects = {
[ "bike/data/body/Mass" ] = { "0.00000" },
[ "bike/data/lean/rate/TorqueMax" ] = { "0.00000" },
[ "bike/data/lean/rate/Damping" ] = { "0.00000" },
[ "bike/data/lean/angle/SteerBias" ] = { "0.00000" }
    }
  },

  -- # FRAME - FAIRING

    frame_fairing_250_stage_0 = {
    name = "",
    description = "",
    stage =0,
    staff_level =0,
    category = "frame",
    type =1,
    time =0,
    championship = "c250",
    effects = {
[ "bike/data/aerodynamic/BikeDragCoeff" ] = { "0.03825" },
[ "bike/data/aerodynamic/BikeReferenceArea" ] = { "0.03825" },
[ "bike/data/aerodynamic/RiderReferenceArea" ] = { "0.04500" }
    }
  },

    frame_fairing_250_stage_1 = {
    name = [[GRENAJ]],
    description = [[Grenaj ve motosiklet biçimindeki temel iyileştirmeler, sürücü eğildiğinde havanın sırtı üzerinden daha düzgün akmasını sağlar.]],
    stage =1,
    staff_level =11,
    category = "frame",
    type =1,
    time =30,
    championship = "c250",
    effects = {
[ "bike/data/aerodynamic/BikeDragCoeff" ] = { "0.03443" },
[ "bike/data/aerodynamic/BikeReferenceArea" ] = { "0.03443" },
[ "bike/data/aerodynamic/RiderReferenceArea" ] = { "0.04050" }
    }
  },

    frame_fairing_250_stage_2 = {
    name = [[GRENAJ]],
    description = [[Grenajdaki orta düzey değişiklikler, motosikletin önemli bölgelerindeki aerodinamik verimliliği artırır.]],
    stage =2,
    staff_level =13,
    category = "frame",
    type =1,
    time =40,
    championship = "c250",
    effects = {
[ "bike/data/aerodynamic/BikeDragCoeff" ] = { "0.02807" },
[ "bike/data/aerodynamic/BikeReferenceArea" ] = { "0.02807" },
[ "bike/data/aerodynamic/RiderReferenceArea" ] = { "0.03303" }
    }
  },

    frame_fairing_250_stage_3 = {
    name = [[GRENAJ]],
    description = [[Bileşenlerin aşırı ısınmasını önlemek için ısı dağılımına odaklanan ek grenaj iyileştirmeleri.]],
    stage =3,
    staff_level =15,
    category = "frame",
    type =1,
    time =50,
    championship = "c250",
    effects = {
[ "bike/data/aerodynamic/BikeDragCoeff" ] = { "0.01752" },
[ "bike/data/aerodynamic/BikeReferenceArea" ] = { "0.01752" },
[ "bike/data/aerodynamic/RiderReferenceArea" ] = { "0.02061" }
    }
  },

    frame_fairing_250_stage_4 = {
    name = [[GRENAJ]],
    description = [[Motosikletin aerodinamik verimliliğini daha da artırmak için rüzgâr tüneli araştırması yapılır.]],
    stage =4,
    staff_level =17,
    category = "frame",
    type =1,
    time =60,
    championship = "c250",
    effects = {
[ "bike/data/aerodynamic/BikeDragCoeff" ] = { "0.00000" },
[ "bike/data/aerodynamic/BikeReferenceArea" ] = { "0.00000" },
[ "bike/data/aerodynamic/RiderReferenceArea" ] = { "0.00000" }
    }
  },

  -- # FRAME - SWING ARM

    frame_swingarm_250_stage_0 = {
    name = "",
    description = "",
    stage =0,
    staff_level =0,
    category = "frame",
    type =2,
    time =0,
    championship = "c250",
    effects = {
[ "bike/data/DrivingAids/RearBrakeControl" ] = { "-0.15000" },
[ "bike/data/DrivingAids/RearBrakeControlSetupMax" ] = { "-0.30000" },
[ "bike/data/DrivingAids/RearBrakeControlSetupMin" ] = { "0.00000" },
[ "bike/data/lean/rate/TorqueMax" ] = { "-0.01500" },
[ "bike/data/lean/rate/Damping" ] = { "0.06000" },
[ "bike/data/lean/angle/SteerBias" ] = { "0.00000" },
[ "bike/data/NormalLoadEQ/DampingRate" ] = { "-0.30000" },
[ "bike/data/suspensions/1/Stiffness" ] = { "7.50000" },
[ "bike/data/suspensions/1/StiffnessSetupMax" ] = { "7.50000" },
[ "bike/data/suspensions/1/StiffnessSetupMin" ] = { "7.50000" },
[ "bike/data/suspensions/1/CompressionDamping" ] = { "-1.50000" },
[ "bike/data/suspensions/1/CompressionDampingSetupMax" ] = { "-1.00000" },
[ "bike/data/suspensions/1/CompressionDampingSetupMin" ] = { "-2.00000" },
[ "bike/data/suspensions/1/ReboundDamping" ] = { "-1.00000" },
[ "bike/data/suspensions/1/ReboundDampingSetupMin" ] = { "-0.50000" },
[ "bike/data/suspensions/1/ReboundDampingSetupMax" ] = { "-1.50000" }
    }
  },

    frame_swingarm_250_stage_1 = {
    name = [[SALINCAK]],
    description = [[Salıncak, zincir torkunun oluşturduğu çok büyük baskı kuvvetlerine dayanmalıdır. Daha sağlam malzemeler kullanılması salıncağın dayanıklılığını artırır.]],
    stage =1,
    staff_level =12,
    category = "frame",
    type =2,
    time =30,
    championship = "c250",
    effects = {
[ "bike/data/DrivingAids/RearBrakeControl" ] = { "-0.07503" },
[ "bike/data/DrivingAids/RearBrakeControlSetupMax" ] = { "-0.15006" },
[ "bike/data/DrivingAids/RearBrakeControlSetupMin" ] = { "0.00000" },
[ "bike/data/lean/rate/TorqueMax" ] = { "-0.00750" },
[ "bike/data/lean/rate/Damping" ] = { "0.03001" },
[ "bike/data/lean/angle/SteerBias" ] = { "0.00000" },
[ "bike/data/NormalLoadEQ/DampingRate" ] = { "-0.15006" },
[ "bike/data/suspensions/1/Stiffness" ] = { "3.75150" },
[ "bike/data/suspensions/1/StiffnessSetupMax" ] = { "3.75150" },
[ "bike/data/suspensions/1/StiffnessSetupMin" ] = { "3.75150" },
[ "bike/data/suspensions/1/CompressionDamping" ] = { "-0.75030" },
[ "bike/data/suspensions/1/CompressionDampingSetupMax" ] = { "-0.50020" },
[ "bike/data/suspensions/1/CompressionDampingSetupMin" ] = { "-1.00040" },
[ "bike/data/suspensions/1/ReboundDamping" ] = { "-0.50020" },
[ "bike/data/suspensions/1/ReboundDampingSetupMin" ] = { "-0.25010" },
[ "bike/data/suspensions/1/ReboundDampingSetupMax" ] = { "-0.75030" }
    }
  },

    frame_swingarm_250_stage_2 = {
    name = [[SALINCAK]],
    description = [[Salıncak süspansiyonundaki iyileştirmeler, viraj çıkışında hızlanırken daha yüksek kontrol sağlar.]],
    stage =2,
    staff_level =16,
    category = "frame",
    type =2,
    time =40,
    championship = "c250",
    effects = {
[ "bike/data/DrivingAids/RearBrakeControl" ] = { "0.00000" },
[ "bike/data/DrivingAids/RearBrakeControlSetupMax" ] = { "0.00000" },
[ "bike/data/DrivingAids/RearBrakeControlSetupMin" ] = { "0.00000" },
[ "bike/data/lean/rate/TorqueMax" ] = { "0.00000" },
[ "bike/data/lean/rate/Damping" ] = { "0.00000" },
[ "bike/data/lean/angle/SteerBias" ] = { "0.00000" },
[ "bike/data/NormalLoadEQ/DampingRate" ] = { "0.00000" },
[ "bike/data/suspensions/1/Stiffness" ] = { "0.00000" },
[ "bike/data/suspensions/1/StiffnessSetupMax" ] = { "0.00000" },
[ "bike/data/suspensions/1/StiffnessSetupMin" ] = { "0.00000" },
[ "bike/data/suspensions/1/CompressionDamping" ] = { "0.00000" },
[ "bike/data/suspensions/1/CompressionDampingSetupMax" ] = { "0.00000" },
[ "bike/data/suspensions/1/CompressionDampingSetupMin" ] = { "0.00000" },
[ "bike/data/suspensions/1/ReboundDamping" ] = { "0.00000" },
[ "bike/data/suspensions/1/ReboundDampingSetupMin" ] = { "0.00000" },
[ "bike/data/suspensions/1/ReboundDampingSetupMax" ] = { "0.00000" }
    }
  },

  -- # FRAME - SHOCK ABSORBERS

    frame_shock_absorbers_250_stage_0 = {
    name = "",
    description = "",
    stage =0,
    staff_level =0,
    category = "frame",
    type =3,
    time =0,
    championship = "c250",
    effects = {
[ "bike/data/DrivingAids/FrontBrakeControl" ] = { "-0.15000" },
[ "bike/data/DrivingAids/FrontBrakeControlSetupMax" ] = { "-0.30000" },
[ "bike/data/DrivingAids/FrontBrakeControlSetupMin" ] = { "0.00000" },
[ "bike/data/lean/rate/TorqueMax" ] = { "-0.01000" },
[ "bike/data/lean/rate/Damping" ] = { "0.06000" },
[ "bike/data/lean/angle/SteerBias" ] = { "0.00000" },
[ "bike/data/NormalLoadEQ/DampingRate" ] = { "-1.05000" },
[ "bike/data/suspensions/0/Stiffness" ] = { "15.00000" },
[ "bike/data/suspensions/0/StiffnessSetupMax" ] = { "15.00000" },
[ "bike/data/suspensions/0/StiffnessSetupMin" ] = { "15.00000" },
[ "bike/data/suspensions/0/CompressionDamping" ] = { "-3.00000" },
[ "bike/data/suspensions/0/CompressionDampingSetupMax" ] = { "-2.00000" },
[ "bike/data/suspensions/0/CompressionDampingSetupMin" ] = { "-4.00000" },
[ "bike/data/suspensions/0/ReboundDamping" ] = { "-2.00000" },
[ "bike/data/suspensions/0/ReboundDampingSetupMax" ] = { "-1.00000" },
[ "bike/data/suspensions/0/ReboundDampingSetupMin" ] = { "-3.00000" },
[ "bike/data/suspensions/1/Stiffness" ] = { "7.50000" },
[ "bike/data/suspensions/1/StiffnessSetupMax" ] = { "7.50000" },
[ "bike/data/suspensions/1/StiffnessSetupMin" ] = { "7.50000" },
[ "bike/data/suspensions/1/CompressionDamping" ] = { "-1.50000" },
[ "bike/data/suspensions/1/CompressionDampingSetupMax" ] = { "-1.00000" },
[ "bike/data/suspensions/1/CompressionDampingSetupMin" ] = { "-2.00000" },
[ "bike/data/suspensions/1/ReboundDamping" ] = { "-1.00000" },
[ "bike/data/suspensions/1/ReboundDampingSetupMin" ] = { "-0.50000" },
[ "bike/data/suspensions/1/ReboundDampingSetupMax" ] = { "-1.50000" },
[ "bike/data/setup/unlock/Suspension" ] = { "-1" }
    }
  },

    frame_shock_absorbers_250_stage_1 = {
    name = [[AMORTİSÖRLER]],
    description = [[Amortisörleri daha gelişmiş bir süspansiyon sistemiyle değiştirerek frenleme ve viraj dengesini artırır.]],
    stage =1,
    staff_level =13,
    category = "frame",
    type =3,
    time =30,
    championship = "c250",
    effects = {
[ "bike/data/DrivingAids/FrontBrakeControl" ] = { "-0.13500" },
[ "bike/data/DrivingAids/FrontBrakeControlSetupMax" ] = { "-0.27000" },
[ "bike/data/DrivingAids/FrontBrakeControlSetupMin" ] = { "0.00000" },
[ "bike/data/lean/rate/TorqueMax" ] = { "-0.00900" },
[ "bike/data/lean/rate/Damping" ] = { "0.05400" },
[ "bike/data/lean/angle/SteerBias" ] = { "0.00000" },
[ "bike/data/NormalLoadEQ/DampingRate" ] = { "-0.94500" },
[ "bike/data/suspensions/0/Stiffness" ] = { "13.50000" },
[ "bike/data/suspensions/0/StiffnessSetupMax" ] = { "13.50000" },
[ "bike/data/suspensions/0/StiffnessSetupMin" ] = { "13.50000" },
[ "bike/data/suspensions/0/CompressionDamping" ] = { "-2.70000" },
[ "bike/data/suspensions/0/CompressionDampingSetupMax" ] = { "-1.80000" },
[ "bike/data/suspensions/0/CompressionDampingSetupMin" ] = { "-3.60000" },
[ "bike/data/suspensions/0/ReboundDamping" ] = { "-1.80000" },
[ "bike/data/suspensions/0/ReboundDampingSetupMax" ] = { "-0.90000" },
[ "bike/data/suspensions/0/ReboundDampingSetupMin" ] = { "-2.70000" },
[ "bike/data/suspensions/1/Stiffness" ] = { "6.75000" },
[ "bike/data/suspensions/1/StiffnessSetupMax" ] = { "6.75000" },
[ "bike/data/suspensions/1/StiffnessSetupMin" ] = { "6.75000" },
[ "bike/data/suspensions/1/CompressionDamping" ] = { "-1.35000" },
[ "bike/data/suspensions/1/CompressionDampingSetupMax" ] = { "-0.90000" },
[ "bike/data/suspensions/1/CompressionDampingSetupMin" ] = { "-1.80000" },
[ "bike/data/suspensions/1/ReboundDamping" ] = { "-0.90000" },
[ "bike/data/suspensions/1/ReboundDampingSetupMin" ] = { "-0.45000" },
[ "bike/data/suspensions/1/ReboundDampingSetupMax" ] = { "-1.35000" },
[ "bike/data/setup/unlock/Suspension" ] = { "-1" }
    }
  },

    frame_shock_absorbers_250_stage_2 = {
    name = [[AMORTİSÖRLER]],
    description = [[Mevcut süspansiyon sistemini daha iyi performans için yeniden ayarlar.]],
    stage =2,
    staff_level =14,
    category = "frame",
    type =3,
    time =35,
    championship = "c250",
    effects = {
[ "bike/data/DrivingAids/FrontBrakeControl" ] = { "-0.11009" },
[ "bike/data/DrivingAids/FrontBrakeControlSetupMax" ] = { "-0.22017" },
[ "bike/data/DrivingAids/FrontBrakeControlSetupMin" ] = { "0.00000" },
[ "bike/data/lean/rate/TorqueMax" ] = { "-0.00734" },
[ "bike/data/lean/rate/Damping" ] = { "0.04403" },
[ "bike/data/lean/angle/SteerBias" ] = { "0.00000" },
[ "bike/data/NormalLoadEQ/DampingRate" ] = { "-0.77060" },
[ "bike/data/suspensions/0/Stiffness" ] = { "11.00850" },
[ "bike/data/suspensions/0/StiffnessSetupMax" ] = { "11.00850" },
[ "bike/data/suspensions/0/StiffnessSetupMin" ] = { "11.00850" },
[ "bike/data/suspensions/0/CompressionDamping" ] = { "-2.20170" },
[ "bike/data/suspensions/0/CompressionDampingSetupMax" ] = { "-1.46780" },
[ "bike/data/suspensions/0/CompressionDampingSetupMin" ] = { "-2.93560" },
[ "bike/data/suspensions/0/ReboundDamping" ] = { "-1.46780" },
[ "bike/data/suspensions/0/ReboundDampingSetupMax" ] = { "-0.73390" },
[ "bike/data/suspensions/0/ReboundDampingSetupMin" ] = { "-2.20170" },
[ "bike/data/suspensions/1/Stiffness" ] = { "5.50425" },
[ "bike/data/suspensions/1/StiffnessSetupMax" ] = { "5.50425" },
[ "bike/data/suspensions/1/StiffnessSetupMin" ] = { "5.50425" },
[ "bike/data/suspensions/1/CompressionDamping" ] = { "-1.10085" },
[ "bike/data/suspensions/1/CompressionDampingSetupMax" ] = { "-0.73390" },
[ "bike/data/suspensions/1/CompressionDampingSetupMin" ] = { "-1.46780" },
[ "bike/data/suspensions/1/ReboundDamping" ] = { "-0.73390" },
[ "bike/data/suspensions/1/ReboundDampingSetupMin" ] = { "-0.36695" },
[ "bike/data/suspensions/1/ReboundDampingSetupMax" ] = { "-1.10085" },
[ "bike/data/setup/unlock/Suspension" ] = { "-1" }
    }
  },

    frame_shock_absorbers_250_stage_3 = {
    name = [[AMORTİSÖRLER]],
    description = [[Bozuk yüzeylerin darbelerini ve sert frenlemenin oluşturduğu sıkışmayı daha etkili sönümleyen gelişmiş amortisörler.]],
    stage =3,
    staff_level =15,
    category = "frame",
    type =3,
    time =40,
    championship = "c250",
    effects = {
[ "bike/data/DrivingAids/FrontBrakeControl" ] = { "-0.06870" },
[ "bike/data/DrivingAids/FrontBrakeControlSetupMax" ] = { "-0.13740" },
[ "bike/data/DrivingAids/FrontBrakeControlSetupMin" ] = { "0.00000" },
[ "bike/data/lean/rate/TorqueMax" ] = { "-0.00458" },
[ "bike/data/lean/rate/Damping" ] = { "0.02748" },
[ "bike/data/lean/angle/SteerBias" ] = { "0.00000" },
[ "bike/data/NormalLoadEQ/DampingRate" ] = { "-0.48091" },
[ "bike/data/suspensions/0/Stiffness" ] = { "6.87012" },
[ "bike/data/suspensions/0/StiffnessSetupMax" ] = { "6.87012" },
[ "bike/data/suspensions/0/StiffnessSetupMin" ] = { "6.87012" },
[ "bike/data/suspensions/0/CompressionDamping" ] = { "-1.37402" },
[ "bike/data/suspensions/0/CompressionDampingSetupMax" ] = { "-0.91602" },
[ "bike/data/suspensions/0/CompressionDampingSetupMin" ] = { "-1.83203" },
[ "bike/data/suspensions/0/ReboundDamping" ] = { "-0.91602" },
[ "bike/data/suspensions/0/ReboundDampingSetupMax" ] = { "-0.45801" },
[ "bike/data/suspensions/0/ReboundDampingSetupMin" ] = { "-1.37402" },
[ "bike/data/suspensions/1/Stiffness" ] = { "3.43506" },
[ "bike/data/suspensions/1/StiffnessSetupMax" ] = { "3.43506" },
[ "bike/data/suspensions/1/StiffnessSetupMin" ] = { "3.43506" },
[ "bike/data/suspensions/1/CompressionDamping" ] = { "-0.68701" },
[ "bike/data/suspensions/1/CompressionDampingSetupMax" ] = { "-0.45801" },
[ "bike/data/suspensions/1/CompressionDampingSetupMin" ] = { "-0.91602" },
[ "bike/data/suspensions/1/ReboundDamping" ] = { "-0.45801" },
[ "bike/data/suspensions/1/ReboundDampingSetupMin" ] = { "-0.22900" },
[ "bike/data/suspensions/1/ReboundDampingSetupMax" ] = { "-0.68701" },
[ "bike/data/setup/unlock/Suspension" ] = { "-1" }
    }
  },

    frame_shock_absorbers_250_stage_4 = {
    name = [[AMORTİSÖRLER]],
    description = [[Bu süspansiyon sistemi daha hassas ayarlar sunarak mühendislerinizin daha yüksek denge elde etmesini sağlar.]],
    stage =4,
    staff_level =16,
    category = "frame",
    type =3,
    time =45,
    championship = "c250",
    effects = {
[ "bike/data/DrivingAids/FrontBrakeControl" ] = { "0.00000" },
[ "bike/data/DrivingAids/FrontBrakeControlSetupMax" ] = { "0.00000" },
[ "bike/data/DrivingAids/FrontBrakeControlSetupMin" ] = { "0.00000" },
[ "bike/data/lean/rate/TorqueMax" ] = { "0.00000" },
[ "bike/data/lean/rate/Damping" ] = { "0.00000" },
[ "bike/data/lean/angle/SteerBias" ] = { "0.00000" },
[ "bike/data/NormalLoadEQ/DampingRate" ] = { "0.00000" },
[ "bike/data/suspensions/0/Stiffness" ] = { "0.00000" },
[ "bike/data/suspensions/0/StiffnessSetupMax" ] = { "0.00000" },
[ "bike/data/suspensions/0/StiffnessSetupMin" ] = { "0.00000" },
[ "bike/data/suspensions/0/CompressionDamping" ] = { "0.00000" },
[ "bike/data/suspensions/0/CompressionDampingSetupMax" ] = { "0.00000" },
[ "bike/data/suspensions/0/CompressionDampingSetupMin" ] = { "0.00000" },
[ "bike/data/suspensions/0/ReboundDamping" ] = { "0.00000" },
[ "bike/data/suspensions/0/ReboundDampingSetupMax" ] = { "0.00000" },
[ "bike/data/suspensions/0/ReboundDampingSetupMin" ] = { "0.00000" },
[ "bike/data/suspensions/1/Stiffness" ] = { "0.00000" },
[ "bike/data/suspensions/1/StiffnessSetupMax" ] = { "0.00000" },
[ "bike/data/suspensions/1/StiffnessSetupMin" ] = { "0.00000" },
[ "bike/data/suspensions/1/CompressionDamping" ] = { "0.00000" },
[ "bike/data/suspensions/1/CompressionDampingSetupMax" ] = { "0.00000" },
[ "bike/data/suspensions/1/CompressionDampingSetupMin" ] = { "0.00000" },
[ "bike/data/suspensions/1/ReboundDamping" ] = { "0.00000" },
[ "bike/data/suspensions/1/ReboundDampingSetupMin" ] = { "0.00000" },
[ "bike/data/suspensions/1/ReboundDampingSetupMax" ] = { "0.00000" },
[ "bike/data/setup/unlock/Suspension" ] = { "0" }
    }
  },

  -- # GEARBOX - CLUTCH

    gearbox_clutch_250_stage_0 = {
    name = "",
    description = "",
    stage =0,
    staff_level =0,
    category = "gearbox",
    type =0,
    time =0,
    championship = "c250",
    effects = {
[ "bike/data/body/Mass" ] = { "0.01000" },
[ "bike/data/engine/MaxCompressionTorque" ] = { "-0.05000" },
[ "bike/data/gearbox/ClutchingTime" ] = { "0.06000" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00200" }
    }
  },

    gearbox_clutch_250_stage_1 = {
    name = [[DEBRİYAJ]],
    description = [[Standart debriyajı kaydırmalı debriyaj mekanizmasıyla değiştirerek vites küçültürken motosikleti çok daha dengeli hâle getirir.]],
    stage =1,
    staff_level =13,
    category = "gearbox",
    type =0,
    time =35,
    championship = "c250",
    effects = {
[ "bike/data/body/Mass" ] = { "0.00900" },
[ "bike/data/engine/MaxCompressionTorque" ] = { "-0.04500" },
[ "bike/data/gearbox/ClutchingTime" ] = { "0.05400" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00180" }
    }
  },

    gearbox_clutch_250_stage_2 = {
    name = [[DEBRİYAJ]],
    description = [[Motor frenine ve daha hızlı vites değişimlerine daha iyi dayanan, güçlendirilmiş hafif debriyaj plakaları.]],
    stage =2,
    staff_level =15,
    category = "gearbox",
    type =0,
    time =40,
    championship = "c250",
    effects = {
[ "bike/data/body/Mass" ] = { "0.00734" },
[ "bike/data/engine/MaxCompressionTorque" ] = { "-0.03670" },
[ "bike/data/gearbox/ClutchingTime" ] = { "0.04403" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00147" }
    }
  },

    gearbox_clutch_250_stage_3 = {
    name = [[DEBRİYAJ]],
    description = [[Debriyajdaki ısıyı daha etkili dağıtarak daha da hızlı vites değiştirmeyi sağlayan teknoloji.]],
    stage =3,
    staff_level =17,
    category = "gearbox",
    type =0,
    time =45,
    championship = "c250",
    effects = {
[ "bike/data/body/Mass" ] = { "0.00458" },
[ "bike/data/engine/MaxCompressionTorque" ] = { "-0.02290" },
[ "bike/data/gearbox/ClutchingTime" ] = { "0.02748" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00092" }
    }
  },

    gearbox_clutch_250_stage_4 = {
    name = [[DEBRİYAJ]],
    description = [[Hassas debriyaj ayarı, vites değiştirerek hızlanırken sarsıntıyı ve debriyajın kaçırmasını büyük ölçüde azaltarak dengeyi artırır.]],
    stage =4,
    staff_level =19,
    category = "gearbox",
    type =0,
    time =50,
    championship = "c250",
    effects = {
[ "bike/data/body/Mass" ] = { "0.00000" },
[ "bike/data/engine/MaxCompressionTorque" ] = { "0.00000" },
[ "bike/data/gearbox/ClutchingTime" ] = { "0.00000" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "0.00000" }
    }
  },

  -- # GEARBOX - RATIOS

  gearbox_ratios_250_stage_0 = {
    name = "",
    description = "",
    stage =0,
    staff_level =0,
    category = "gearbox",
    type =1,
    time =0,
    championship = "c250",
    effects = {
[ "bike/data/engine/MaxTorque" ] = { "-0.00800" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00200" },
[ "bike/data/setup/unlock/Gearbox" ] = { "-1" }
    }
  },

    gearbox_ratios_250_stage_1 = {
    name = [[VİTES ORANLARI]],
    description = [[Daha iyi hızlanma sağlayan küçük bir şanzıman iyileştirmesi.]],
    stage =1,
    staff_level =14,
    category = "gearbox",
    type =1,
    time =30,
    championship = "c250",
    effects = {
[ "bike/data/engine/MaxTorque" ] = { "-0.00720" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00180" },
[ "bike/data/setup/unlock/Gearbox" ] = { "-1" }
    }
  },

   gearbox_ratios_250_stage_2 = {
    name = [[VİTES ORANLARI]],
    description = [[Hızlanmayı daha da artırmak için motosiklete en uygun vites düzeni araştırılır.]],
    stage =2,
    staff_level =15,
    category = "gearbox",
    type =1,
    time =40,
    championship = "c250",
    effects = {
[ "bike/data/engine/MaxTorque" ] = { "-0.00587" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00147" },
[ "bike/data/setup/unlock/Gearbox" ] = { "-1" }
    }
  },

    gearbox_ratios_250_stage_3 = {
    name = [[VİTES ORANLARI]],
    description = [[Motorun yüksek torkuna ve daha sert vites değişimlerine dayanmak için çeşitli şanzıman parçaları daha hafif ve sağlam malzemelerle değiştirilir.]],
    stage =3,
    staff_level =16,
    category = "gearbox",
    type =1,
    time =50,
    championship = "c250",
    effects = {
[ "bike/data/engine/MaxTorque" ] = { "-0.00366" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00092" },
[ "bike/data/setup/unlock/Gearbox" ] = { "-1" }
    }
  },

    gearbox_ratios_250_stage_4 = {
    name = [[VİTES ORANLARI]],
    description = [[Tüm devir aralığında güç eğrisinden tam yararlanarak hızlanmayı önemli ölçüde artıran kapsamlı bir şanzıman yenilemesi.]],
    stage =4,
    staff_level =17,
    category = "gearbox",
    type =1,
    time =60,
    championship = "c250",
    effects = {
[ "bike/data/engine/MaxTorque" ] = { "0.00000" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "0.00000" },
[ "bike/data/setup/unlock/Gearbox" ] = { "0" }
    }
  },

  -- # GP ---------------------------------------------------------------------------------------------------------------------

    -- # BRAKES

    brakes_gp_stage_0 = {
    name = "",
    description = "",
    stage =0,
    staff_level =0,
    category = "brakes",
    type =0,
    time =0,
    championship = "gp",
    effects = {
[ "bike/data/body/Mass" ] = { "0.01500" },
[ "bike/data/brakes/0/Force" ] = { "-5.00000" },
[ "bike/data/brakes/1/Force" ] = { "-0.50000" },
[ "bike/data/NormalLoadEQ/DampingRate" ] = { "-0.30000" },
[ "bike/data/DrivingAids/FrontBrakeControl" ] = { "-0.35000" },
[ "bike/data/DrivingAids/FrontBrakeControlSetupMax" ] = { "-0.35000" },
[ "bike/data/DrivingAids/FrontBrakeControlSetupMin" ] = { "0.00000" },
[ "bike/data/DrivingAids/RearBrakeControl" ] = { "-0.07000" },
[ "bike/data/DrivingAids/RearBrakeControlSetupMax" ] = { "-0.14000" },
[ "bike/data/DrivingAids/RearBrakeControlSetupMin" ] = { "0.00000" }
    }
  },

    brakes_gp_stage_1 = {
    name = [[FRENLER]],
    description = [[En uygun sıcaklığa daha çabuk ulaşan hafif diskler.]],
    stage =1,
    staff_level =17,
    category = "brakes",
    type =0,
    time =20,
    championship = "gp",
    effects = {
[ "bike/data/body/Mass" ] = { "0.01350" },
[ "bike/data/brakes/0/Force" ] = { "-4.50000" },
[ "bike/data/brakes/1/Force" ] = { "-0.45000" },
[ "bike/data/NormalLoadEQ/DampingRate" ] = { "-0.27000" },
[ "bike/data/DrivingAids/FrontBrakeControl" ] = { "-0.31500" },
[ "bike/data/DrivingAids/FrontBrakeControlSetupMax" ] = { "-0.31500" },
[ "bike/data/DrivingAids/FrontBrakeControlSetupMin" ] = { "0.00000" },
[ "bike/data/DrivingAids/RearBrakeControl" ] = { "-0.06300" },
[ "bike/data/DrivingAids/RearBrakeControlSetupMax" ] = { "-0.12600" },
[ "bike/data/DrivingAids/RearBrakeControlSetupMin" ] = { "0.00000" }
    }
  },

    brakes_gp_stage_2 = {
    name = [[FRENLER]],
    description = [[Daha yüksek dayanıklılık ve frenleme gücü.]],
    stage =2,
    staff_level =19,
    category = "brakes",
    type =0,
    time =40,
    championship = "gp",
    effects = {
[ "bike/data/body/Mass" ] = { "0.01147" },
[ "bike/data/brakes/0/Force" ] = { "-3.82375" },
[ "bike/data/brakes/1/Force" ] = { "-0.38238" },
[ "bike/data/NormalLoadEQ/DampingRate" ] = { "-0.22943" },
[ "bike/data/DrivingAids/FrontBrakeControl" ] = { "-0.26766" },
[ "bike/data/DrivingAids/FrontBrakeControlSetupMax" ] = { "-0.26766" },
[ "bike/data/DrivingAids/FrontBrakeControlSetupMin" ] = { "0.00000" },
[ "bike/data/DrivingAids/RearBrakeControl" ] = { "-0.05353" },
[ "bike/data/DrivingAids/RearBrakeControlSetupMax" ] = { "-0.10707" },
[ "bike/data/DrivingAids/RearBrakeControlSetupMin" ] = { "0.00000" }
    }
  },

    brakes_gp_stage_3 = {
    name = [[FRENLER]],
    description = [[Fren performansında ek iyileştirme.]],
    stage =3,
    staff_level =21,
    category = "brakes",
    type =0,
    time =60,
    championship = "gp",
    effects = {
[ "bike/data/body/Mass" ] = { "0.00873" },
[ "bike/data/brakes/0/Force" ] = { "-2.90912" },
[ "bike/data/brakes/1/Force" ] = { "-0.29091" },
[ "bike/data/NormalLoadEQ/DampingRate" ] = { "-0.17455" },
[ "bike/data/DrivingAids/FrontBrakeControl" ] = { "-0.20364" },
[ "bike/data/DrivingAids/FrontBrakeControlSetupMax" ] = { "-0.20364" },
[ "bike/data/DrivingAids/FrontBrakeControlSetupMin" ] = { "0.00000" },
[ "bike/data/DrivingAids/RearBrakeControl" ] = { "-0.04073" },
[ "bike/data/DrivingAids/RearBrakeControlSetupMax" ] = { "-0.08146" },
[ "bike/data/DrivingAids/RearBrakeControlSetupMin" ] = { "0.00000" }
    }
  },

    brakes_gp_stage_4 = {
    name = [[FRENLER]],
    description = [[Son derece hafif disk frenler.]],
    stage =4,
    staff_level =23,
    category = "brakes",
    type =0,
    time =80,
    championship = "gp",
    effects = {
[ "bike/data/body/Mass" ] = { "0.00502" },
[ "bike/data/brakes/0/Force" ] = { "-1.67209" },
[ "bike/data/brakes/1/Force" ] = { "-0.16721" },
[ "bike/data/NormalLoadEQ/DampingRate" ] = { "-0.10033" },
[ "bike/data/DrivingAids/FrontBrakeControl" ] = { "-0.11705" },
[ "bike/data/DrivingAids/FrontBrakeControlSetupMax" ] = { "-0.11705" },
[ "bike/data/DrivingAids/FrontBrakeControlSetupMin" ] = { "0.00000" },
[ "bike/data/DrivingAids/RearBrakeControl" ] = { "-0.02341" },
[ "bike/data/DrivingAids/RearBrakeControlSetupMax" ] = { "-0.04682" },
[ "bike/data/DrivingAids/RearBrakeControlSetupMin" ] = { "0.00000" }
    }
  },

    brakes_gp_stage_5 = {
    name = [[FRENLER]],
    description = [[Sürekli üstün performans sunan gelişmiş diskler.]],
    stage =5,
    staff_level =25,
    category = "brakes",
    type =0,
    time =100,
    championship = "gp",
    effects = {
[ "bike/data/body/Mass" ] = { "0.00000" },
[ "bike/data/brakes/0/Force" ] = { "0.00000" },
[ "bike/data/brakes/1/Force" ] = { "0.00000" },
[ "bike/data/NormalLoadEQ/DampingRate" ] = { "0.00000" },
[ "bike/data/DrivingAids/FrontBrakeControl" ] = { "0.00000" },
[ "bike/data/DrivingAids/FrontBrakeControlSetupMax" ] = { "0.00000" },
[ "bike/data/DrivingAids/FrontBrakeControlSetupMin" ] = { "0.00000" },
[ "bike/data/DrivingAids/RearBrakeControl" ] = { "0.00000" },
[ "bike/data/DrivingAids/RearBrakeControlSetupMax" ] = { "0.00000" },
[ "bike/data/DrivingAids/RearBrakeControlSetupMin" ] = { "0.00000" }
    }
  },

  -- # Soft tyres

  tyres_soft_compound_gp_stage_0 = {
    name = "",
    description = "",
    stage =0,
    staff_level =0,
    category = "brakes",
    type =1,
    time =0,
    championship = "gp",
    effects = {
[ "bike/data/DrivingAids/TractionControl" ] = { "-0.10000" },
[ "bike/data/DrivingAids/TractionControlSetupMax" ] = { "-0.25000" },
[ "bike/data/DrivingAids/TractionControlSetupMin" ] = { "0.00000" },
[ "bike/data/wheels/0/Soft/LongitudinalFrictionCoeffMaxCoeff " ] = { "-0.10000" },
[ "bike/data/wheels/0/Soft/LateralFrictionMaxCoeff" ] = { "-0.10000" },
[ "bike/data/wheels/0/Soft/LateralFrictionCoeffLimitMaxCoeff" ] = { "-0.10000" },
[ "bike/data/wheels/1/Soft/LongitudinalFrictionCoeffMaxCoeff " ] = { "-0.10000" },
[ "bike/data/wheels/1/Soft/LateralFrictionMaxCoeff" ] = { "-0.10000" },
[ "bike/data/wheels/1/Soft/LateralFrictionCoeffLimitMaxCoeff" ] = { "-0.10000" }
    }
  },

    tyres_soft_compound_gp_stage_1 = {
    name = [[YUMUŞAK HAMUR]],
    description = [[Yumuşak lastik hamurunun dayanıklılığını ve yol tutuşunu biraz iyileştirir.]],
    stage =1,
    staff_level =20,
    category = "brakes",
    type =1,
    time =60,
    championship = "gp",
    effects = {
[ "bike/data/DrivingAids/TractionControl" ] = { "-0.09000" },
[ "bike/data/DrivingAids/TractionControlSetupMax" ] = { "-0.22500" },
[ "bike/data/DrivingAids/TractionControlSetupMin" ] = { "0.00000" },
[ "bike/data/wheels/0/Soft/LongitudinalFrictionCoeffMaxCoeff " ] = { "-0.09000" },
[ "bike/data/wheels/0/Soft/LateralFrictionMaxCoeff" ] = { "-0.09000" },
[ "bike/data/wheels/0/Soft/LateralFrictionCoeffLimitMaxCoeff" ] = { "-0.09000" },
[ "bike/data/wheels/1/Soft/LongitudinalFrictionCoeffMaxCoeff " ] = { "-0.09000" },
[ "bike/data/wheels/1/Soft/LateralFrictionMaxCoeff" ] = { "-0.09000" },
[ "bike/data/wheels/1/Soft/LateralFrictionCoeffLimitMaxCoeff" ] = { "-0.09000" }
    }
  },

  tyres_soft_compound_gp_stage_2 = {
    name = [[YUMUŞAK HAMUR]],
    description = [[Lastiklerin performansını daha uzun süre korumasını ve daha dayanıklı olmasını sağlayan yeni yumuşak hamur.]],
    stage =2,
    staff_level =22,
    category = "brakes",
    type =1,
    time =70,
    championship = "gp",
    effects = {
[ "bike/data/DrivingAids/TractionControl" ] = { "-0.07648" },
[ "bike/data/DrivingAids/TractionControlSetupMax" ] = { "-0.19119" },
[ "bike/data/DrivingAids/TractionControlSetupMin" ] = { "0.00000" },
[ "bike/data/wheels/0/Soft/LongitudinalFrictionCoeffMaxCoeff " ] = { "-0.07648" },
[ "bike/data/wheels/0/Soft/LateralFrictionMaxCoeff" ] = { "-0.07648" },
[ "bike/data/wheels/0/Soft/LateralFrictionCoeffLimitMaxCoeff" ] = { "-0.07648" },
[ "bike/data/wheels/1/Soft/LongitudinalFrictionCoeffMaxCoeff " ] = { "-0.07648" },
[ "bike/data/wheels/1/Soft/LateralFrictionMaxCoeff" ] = { "-0.07648" },
[ "bike/data/wheels/1/Soft/LateralFrictionCoeffLimitMaxCoeff" ] = { "-0.07648" }
    }
  },

    tyres_soft_compound_gp_stage_3 = {
    name = [[YUMUŞAK HAMUR]],
    description = [[Yumuşak lastik hamurunun yol tutuşunda önemli iyileştirme.]],
    stage =3,
    staff_level =24,
    category = "brakes",
    type =1,
    time =70,
    championship = "gp",
    effects = {
[ "bike/data/DrivingAids/TractionControl" ] = { "-0.05818" },
[ "bike/data/DrivingAids/TractionControlSetupMax" ] = { "-0.14546" },
[ "bike/data/DrivingAids/TractionControlSetupMin" ] = { "0.00000" },
[ "bike/data/wheels/0/Soft/LongitudinalFrictionCoeffMaxCoeff " ] = { "-0.05818" },
[ "bike/data/wheels/0/Soft/LateralFrictionMaxCoeff" ] = { "-0.05818" },
[ "bike/data/wheels/0/Soft/LateralFrictionCoeffLimitMaxCoeff" ] = { "-0.05818" },
[ "bike/data/wheels/1/Soft/LongitudinalFrictionCoeffMaxCoeff " ] = { "-0.05818" },
[ "bike/data/wheels/1/Soft/LateralFrictionMaxCoeff" ] = { "-0.05818" },
[ "bike/data/wheels/1/Soft/LateralFrictionCoeffLimitMaxCoeff" ] = { "-0.05818" }
    }
  },

    tyres_soft_compound_gp_stage_4 = {
    name = [[YUMUŞAK HAMUR]],
    description = [[Yumuşak lastik hamurunun dayanıklılığını artırır.]],
    stage =4,
    staff_level =26,
    category = "brakes",
    type =1,
    time =80,
    championship = "gp",
    effects = {
[ "bike/data/DrivingAids/TractionControl" ] = { "-0.03344" },
[ "bike/data/DrivingAids/TractionControlSetupMax" ] = { "-0.08360" },
[ "bike/data/DrivingAids/TractionControlSetupMin" ] = { "0.00000" },
[ "bike/data/wheels/0/Soft/LongitudinalFrictionCoeffMaxCoeff " ] = { "-0.03344" },
[ "bike/data/wheels/0/Soft/LateralFrictionMaxCoeff" ] = { "-0.03344" },
[ "bike/data/wheels/0/Soft/LateralFrictionCoeffLimitMaxCoeff" ] = { "-0.03344" },
[ "bike/data/wheels/1/Soft/LongitudinalFrictionCoeffMaxCoeff " ] = { "-0.03344" },
[ "bike/data/wheels/1/Soft/LateralFrictionMaxCoeff" ] = { "-0.03344" },
[ "bike/data/wheels/1/Soft/LateralFrictionCoeffLimitMaxCoeff" ] = { "-0.03344" }
    }
  },

    tyres_soft_compound_gp_stage_5 = {
    name = [[YUMUŞAK HAMUR]],
    description = [[Lastik teknolojisinin sınırlarını zorlar. En iyi yumuşak lastik hamuru.]],
    stage =5,
    staff_level =28,
    category = "brakes",
    type =1,
    time =80,
    championship = "gp",
    effects = {
[ "bike/data/DrivingAids/TractionControl" ] = { "0.00000" },
[ "bike/data/DrivingAids/TractionControlSetupMax" ] = { "0.00000" },
[ "bike/data/DrivingAids/TractionControlSetupMin" ] = { "0.00000" },
[ "bike/data/wheels/0/Soft/LongitudinalFrictionCoeffMaxCoeff " ] = { "0.00000" },
[ "bike/data/wheels/0/Soft/LateralFrictionMaxCoeff" ] = { "0.00000" },
[ "bike/data/wheels/0/Soft/LateralFrictionCoeffLimitMaxCoeff" ] = { "0.00000" },
[ "bike/data/wheels/1/Soft/LongitudinalFrictionCoeffMaxCoeff " ] = { "0.00000" },
[ "bike/data/wheels/1/Soft/LateralFrictionMaxCoeff" ] = { "0.00000" },
[ "bike/data/wheels/1/Soft/LateralFrictionCoeffLimitMaxCoeff" ] = { "0.00000" }
    }
  },

    -- # Hard tyres

    tyres_hard_compound_gp_stage_0 = {
    name = "",
    description = "",
    stage =0,
    staff_level =0,
    category = "brakes",
    type =2,
    time =0,
    championship = "gp",
    effects = {
[ "bike/data/DrivingAids/TractionControl" ] = { "-0.10000" },
[ "bike/data/DrivingAids/TractionControlSetupMax" ] = { "-0.25000" },
[ "bike/data/DrivingAids/TractionControlSetupMin" ] = { "0.00000" },
[ "bike/data/wheels/0/Hard/LongitudinalFrictionCoeffMaxCoeff " ] = { "-0.10000" },
[ "bike/data/wheels/0/Hard/LateralFrictionMaxCoeff" ] = { "-0.10000" },
[ "bike/data/wheels/0/Hard/LateralFrictionCoeffLimitMaxCoeff" ] = { "-0.10000" },
[ "bike/data/wheels/1/Hard/LongitudinalFrictionCoeffMaxCoeff " ] = { "-0.10000" },
[ "bike/data/wheels/1/Hard/LateralFrictionMaxCoeff" ] = { "-0.10000" },
[ "bike/data/wheels/1/Hard/LateralFrictionCoeffLimitMaxCoeff" ] = { "-0.10000" }
    }
  },

    tyres_hard_compound_gp_stage_1 = {
    name = [[SERT HAMUR]],
    description = [[Sert lastik hamurunun dayanıklılığını ve yol tutuşunu biraz iyileştirir.]],
    stage =1,
    staff_level =20,
    category = "brakes",
    type =2,
    time =60,
    championship = "gp",
    effects = {
[ "bike/data/DrivingAids/TractionControl" ] = { "-0.09000" },
[ "bike/data/DrivingAids/TractionControlSetupMax" ] = { "-0.22500" },
[ "bike/data/DrivingAids/TractionControlSetupMin" ] = { "0.00000" },
[ "bike/data/wheels/0/Hard/LongitudinalFrictionCoeffMaxCoeff " ] = { "-0.09000" },
[ "bike/data/wheels/0/Hard/LateralFrictionMaxCoeff" ] = { "-0.09000" },
[ "bike/data/wheels/0/Hard/LateralFrictionCoeffLimitMaxCoeff" ] = { "-0.09000" },
[ "bike/data/wheels/1/Hard/LongitudinalFrictionCoeffMaxCoeff " ] = { "-0.09000" },
[ "bike/data/wheels/1/Hard/LateralFrictionMaxCoeff" ] = { "-0.09000" },
[ "bike/data/wheels/1/Hard/LateralFrictionCoeffLimitMaxCoeff" ] = { "-0.09000" }
    }
  },

    tyres_hard_compound_gp_stage_2 = {
    name = [[SERT HAMUR]],
    description = [[Lastiklerin performansını daha uzun süre korumasını ve daha dayanıklı olmasını sağlayan yeni sert hamur.]],
    stage =2,
    staff_level =22,
    category = "brakes",
    type =2,
    time =70,
    championship = "gp",
    effects = {
[ "bike/data/DrivingAids/TractionControl" ] = { "-0.07648" },
[ "bike/data/DrivingAids/TractionControlSetupMax" ] = { "-0.19119" },
[ "bike/data/DrivingAids/TractionControlSetupMin" ] = { "0.00000" },
[ "bike/data/wheels/0/Hard/LongitudinalFrictionCoeffMaxCoeff " ] = { "-0.07648" },
[ "bike/data/wheels/0/Hard/LateralFrictionMaxCoeff" ] = { "-0.07648" },
[ "bike/data/wheels/0/Hard/LateralFrictionCoeffLimitMaxCoeff" ] = { "-0.07648" },
[ "bike/data/wheels/1/Hard/LongitudinalFrictionCoeffMaxCoeff " ] = { "-0.07648" },
[ "bike/data/wheels/1/Hard/LateralFrictionMaxCoeff" ] = { "-0.07648" },
[ "bike/data/wheels/1/Hard/LateralFrictionCoeffLimitMaxCoeff" ] = { "-0.07648" }
    }
  },

    tyres_hard_compound_gp_stage_3 = {
    name = [[SERT HAMUR]],
    description = [[Sert lastik hamurunun yol tutuşunda önemli iyileştirme.]],
    stage =3,
    staff_level =24,
    category = "brakes",
    type =2,
    time =70,
    championship = "gp",
    effects = {
[ "bike/data/DrivingAids/TractionControl" ] = { "-0.05818" },
[ "bike/data/DrivingAids/TractionControlSetupMax" ] = { "-0.14546" },
[ "bike/data/DrivingAids/TractionControlSetupMin" ] = { "0.00000" },
[ "bike/data/wheels/0/Hard/LongitudinalFrictionCoeffMaxCoeff " ] = { "-0.05818" },
[ "bike/data/wheels/0/Hard/LateralFrictionMaxCoeff" ] = { "-0.05818" },
[ "bike/data/wheels/0/Hard/LateralFrictionCoeffLimitMaxCoeff" ] = { "-0.05818" },
[ "bike/data/wheels/1/Hard/LongitudinalFrictionCoeffMaxCoeff " ] = { "-0.05818" },
[ "bike/data/wheels/1/Hard/LateralFrictionMaxCoeff" ] = { "-0.05818" },
[ "bike/data/wheels/1/Hard/LateralFrictionCoeffLimitMaxCoeff" ] = { "-0.05818" }
    }
  },

    tyres_hard_compound_gp_stage_4 = {
    name = [[SERT HAMUR]],
    description = [[Sert lastik hamurunun dayanıklılığını artırır.]],
    stage =4,
    staff_level =26,
    category = "brakes",
    type =2,
    time =80,
    championship = "gp",
    effects = {
[ "bike/data/DrivingAids/TractionControl" ] = { "-0.03344" },
[ "bike/data/DrivingAids/TractionControlSetupMax" ] = { "-0.08360" },
[ "bike/data/DrivingAids/TractionControlSetupMin" ] = { "0.00000" },
[ "bike/data/wheels/0/Hard/LongitudinalFrictionCoeffMaxCoeff " ] = { "-0.03344" },
[ "bike/data/wheels/0/Hard/LateralFrictionMaxCoeff" ] = { "-0.03344" },
[ "bike/data/wheels/0/Hard/LateralFrictionCoeffLimitMaxCoeff" ] = { "-0.03344" },
[ "bike/data/wheels/1/Hard/LongitudinalFrictionCoeffMaxCoeff " ] = { "-0.03344" },
[ "bike/data/wheels/1/Hard/LateralFrictionMaxCoeff" ] = { "-0.03344" },
[ "bike/data/wheels/1/Hard/LateralFrictionCoeffLimitMaxCoeff" ] = { "-0.03344" }
    }
  },

    tyres_hard_compound_gp_stage_5 = {
    name = [[SERT HAMUR]],
    description = [[Lastik teknolojisinin sınırlarını zorlar. En iyi sert lastik hamuru.]],
    stage =5,
    staff_level =28,
    category = "brakes",
    type =2,
    time =80,
    championship = "gp",
    effects = {
[ "bike/data/DrivingAids/TractionControl" ] = { "0.00000" },
[ "bike/data/DrivingAids/TractionControlSetupMax" ] = { "0.00000" },
[ "bike/data/DrivingAids/TractionControlSetupMin" ] = { "0.00000" },
[ "bike/data/wheels/0/Hard/LongitudinalFrictionCoeffMaxCoeff " ] = { "0.00000" },
[ "bike/data/wheels/0/Hard/LateralFrictionMaxCoeff" ] = { "0.00000" },
[ "bike/data/wheels/0/Hard/LateralFrictionCoeffLimitMaxCoeff" ] = { "0.00000" },
[ "bike/data/wheels/1/Hard/LongitudinalFrictionCoeffMaxCoeff " ] = { "0.00000" },
[ "bike/data/wheels/1/Hard/LateralFrictionMaxCoeff" ] = { "0.00000" },
[ "bike/data/wheels/1/Hard/LateralFrictionCoeffLimitMaxCoeff" ] = { "0.00000" }
    }
  },

  -- # ENGINE

  -- # ENGINE - BALANCING

    engine_balancing_gp_stage_0 = {
    name = "",
    description = "",
    stage =0,
    staff_level =0,
    category = "engine",
    type =0,
    time =0,
    championship = "gp",
    effects = {
[ "bike/data/engine/MaxTorque" ] = { "-0.01200" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00040" }
    }
  },

    engine_balancing_gp_stage_1 = {
    name = [[BALANS AYARI]],
    description = [[Motorun hassas biçimde dengelenmesi; aşırı ses, titreşim ve sürtünmeden kaynaklanan enerji kaybını azaltarak verimliliği artırır.]],
    stage =1,
    staff_level =20,
    category = "engine",
    type =0,
    time =40,
    championship = "gp",
    effects = {
[ "bike/data/engine/MaxTorque" ] = { "-0.00600" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00020" }
    }
  },

    engine_balancing_gp_stage_2 = {
    name = [[BALANS AYARI]],
    description = [[Tüm devir aralığında daha düzenli bir güç dağılımı sağlayarak motorun performansını biraz artırır.]],
    stage =2,
    staff_level =21,
    category = "engine",
    type =0,
    time =60,
    championship = "gp",
    effects = {
[ "bike/data/engine/MaxTorque" ] = { "0.00000" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "0.00000" }
    }
  },

  -- # ENGINE - RADIATOR

    engine_radiator_gp_stage_0 = {
    name = "",
    description = "",
    stage =0,
    staff_level =0,
    category = "engine",
    type =1,
    time =0,
    championship = "gp",
    effects = {
[ "bike/data/aerodynamic/BikeDragCoeff" ] = { "0.00600" },
[ "bike/data/aerodynamic/BikeReferenceArea" ] = { "0.00600" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00040" },
[ "bike/data/engine/MaxTorque" ] = { "-0.01200" }
    }
  },

    engine_radiator_gp_stage_1 = {
    name = [[RADYATÖR]],
    description = [[Radyatörün hava girişleri iyileştirilerek hava direnci azaltılır ve havalandırma artırılır.]],
    stage =1,
    staff_level =20,
    category = "engine",
    type =1,
    time =40,
    championship = "gp",
    effects = {
[ "bike/data/aerodynamic/BikeDragCoeff" ] = { "0.00300" },
[ "bike/data/aerodynamic/BikeReferenceArea" ] = { "0.00300" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00020" },
[ "bike/data/engine/MaxTorque" ] = { "-0.00600" }
    }
  },

    engine_radiator_gp_stage_2 = {
    name = [[RADYATÖR]],
    description = [[Radyatörün aerodinamiğini ve motoru soğutma performansını daha da iyileştirir.]],
    stage =2,
    staff_level =24,
    category = "engine",
    type =1,
    time =50,
    championship = "gp",
    effects = {
[ "bike/data/aerodynamic/BikeDragCoeff" ] = { "0.00000" },
[ "bike/data/aerodynamic/BikeReferenceArea" ] = { "0.00000" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "0.00000" },
[ "bike/data/engine/MaxTorque" ] = { "0.00000" }
    }
  },

  -- # ENGINE - CRANKSHAFT

    engine_crankshaft_gp_stage_0 = {
    name = "",
    description = "",
    stage =0,
    staff_level =0,
    category = "engine",
    type =2,
    time =0,
    championship = "gp",
    effects = {
[ "bike/data/engine/MaxTorque" ] = { "0.00600" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00040" },
[ "bike/data/lean/rate/TorqueMax" ] = { "-0.30000" },
[ "bike/data/lean/rate/Damping" ] = { "0.07500" },
[ "bike/data/lean/angle/SteerBias" ] = { "0.00000" }
    }
  },

    engine_crankshaft_gp_stage_1 = {
    name = [[KRANK MİLİ]],
    description = [[Motosiklet üzerindeki jiroskopik kuvvetleri dengelemek için tekerleklerin ters yönünde dönen bir krank mili sistemi geliştirir. Bir miktar motor gücünden ödün vererek motosikletin viraj tepkisini iyileştirir.]],
    stage =1,
    staff_level =25,
    category = "engine",
    type =2,
    time =80,
    championship = "gp",
    effects = {
[ "bike/data/engine/MaxTorque" ] = { "0.00000" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "0.00000" },
[ "bike/data/lean/rate/TorqueMax" ] = { "0.00000" },
[ "bike/data/lean/rate/Damping" ] = { "0.00000" },
[ "bike/data/lean/angle/SteerBias" ] = { "0.00000" }
    }
  },

  -- # ENGINE - PORT POLISH

    engine_portpolish_gp_stage_0 = {
    name = "",
    description = "",
    stage =0,
    staff_level =0,
    category = "engine",
    type =3,
    time =0,
    championship = "gp",
    effects = {
[ "bike/data/engine/MaxTorque" ] = { "-0.00600" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00025" }
    }
  },

    engine_portpolish_gp_stage_1 = {
    name = [[KANAL PARLATMA]],
    description = [[Motorun emme kanalları parlatılarak hava akışı düzgünleştirilir.]],
    stage =1,
    staff_level =18,
    category = "engine",
    type =3,
    time =20,
    championship = "gp",
    effects = {
[ "bike/data/engine/MaxTorque" ] = { "-0.00300" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00013" }
    }
  },

    engine_portpolish_gp_stage_2 = {
    name = [[KANAL PARLATMA]],
    description = [[En yüksek performans için egzoz kanalları da parlatılır.]],
    stage =2,
    staff_level =20,
    category = "engine",
    type =3,
    time =30,
    championship = "gp",
    effects = {
[ "bike/data/engine/MaxTorque" ] = { "0.00000" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "0.00000" }
    }
  },

  -- # ENGINE - VALVES

    engine_valves_gp_stage_0 = {
    name = "",
    description = "",
    stage =0,
    staff_level =0,
    category = "engine",
    type =4,
    time =0,
    championship = "gp",
    effects = {
[ "bike/data/engine/MaxTorque" ] = { "-0.03000" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00090" },
[ "bike/data/DrivingAids/TractionControl" ] = { "-0.10000" },
[ "bike/data/DrivingAids/TractionControlSetupMax" ] = { "-0.25000" },
[ "bike/data/DrivingAids/TractionControlSetupMin" ] = { "0.00000" }
    }
  },

    engine_valves_gp_stage_1 = {
    name = [[SUPAPLAR]],
    description = [[Daha güvenilir yaylı supaplar geliştirir.]],
    stage =1,
    staff_level =17,
    category = "engine",
    type =4,
    time =30,
    championship = "gp",
    effects = {
[ "bike/data/engine/MaxTorque" ] = { "-0.02700" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00081" },
[ "bike/data/DrivingAids/TractionControl" ] = { "-0.09000" },
[ "bike/data/DrivingAids/TractionControlSetupMax" ] = { "-0.22500" },
[ "bike/data/DrivingAids/TractionControlSetupMin" ] = { "0.00000" }
    }
  },

    engine_valves_gp_stage_2 = {
    name = [[SUPAPLAR]],
    description = [[Gelişmiş silindir kapağı teknolojisi. Yayların yerini külbütör mekanizmaları alır. Motor performansı artar.]],
    stage =2,
    staff_level =20,
    category = "engine",
    type =4,
    time =50,
    championship = "gp",
    effects = {
[ "bike/data/engine/MaxTorque" ] = { "-0.02294" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00069" },
[ "bike/data/DrivingAids/TractionControl" ] = { "-0.07648" },
[ "bike/data/DrivingAids/TractionControlSetupMax" ] = { "-0.19119" },
[ "bike/data/DrivingAids/TractionControlSetupMin" ] = { "0.00000" }
    }
  },

   engine_valves_gp_stage_3 = {
    name = [[SUPAPLAR]],
    description = [[Supapların açılma ve kapanma hızı artırılarak motor gücü yükseltilir.]],
    stage =3,
    staff_level =23,
    category = "engine",
    type =4,
    time =70,
    championship = "gp",
    effects = {
[ "bike/data/engine/MaxTorque" ] = { "-0.01745" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00052" },
[ "bike/data/DrivingAids/TractionControl" ] = { "-0.05818" },
[ "bike/data/DrivingAids/TractionControlSetupMax" ] = { "-0.14546" },
[ "bike/data/DrivingAids/TractionControlSetupMin" ] = { "0.00000" }
    }
  },

    engine_valves_gp_stage_4 = {
    name = [[SUPAPLAR]],
    description = [[Külbütör mekanizmaları pnömatik supaplarla değiştirilir. Performans ve güvenilirlik daha da artar.]],
    stage =4,
    staff_level =26,
    category = "engine",
    type =4,
    time =90,
    championship = "gp",
    effects = {
[ "bike/data/engine/MaxTorque" ] = { "-0.01003" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00030" },
[ "bike/data/DrivingAids/TractionControl" ] = { "-0.03344" },
[ "bike/data/DrivingAids/TractionControlSetupMax" ] = { "-0.08360" },
[ "bike/data/DrivingAids/TractionControlSetupMin" ] = { "0.00000" }
    }
  },

    engine_valves_gp_stage_5 = {
    name = [[SUPAPLAR]],
    description = [[Emme ve egzoz evrelerinin çakışmasını en aza indiren hassas supap ayarıyla yakıt verimliliği ve güç artırılır.]],
    stage =5,
    staff_level =29,
    category = "engine",
    type =4,
    time =110,
    championship = "gp",
    effects = {
[ "bike/data/engine/MaxTorque" ] = { "0.00000" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "0.00000" },
[ "bike/data/DrivingAids/TractionControl" ] = { "0.00000" },
[ "bike/data/DrivingAids/TractionControlSetupMax" ] = { "0.00000" },
[ "bike/data/DrivingAids/TractionControlSetupMin" ] = { "0.00000" }
    }
  },

  -- # EXHAUST - MANIFOLD

    exhaust_manifold_gp_stage_0 = {
    name = "",
    description = "",
    stage =0,
    staff_level =0,
    category = "exhaust",
    type =0,
    time =0,
    championship = "gp",
    effects = {
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00075" },
[ "bike/data/engine/MaxTorque" ] = { "-0.01920" },
[ "bike/data/body/Mass" ] = { "0.03000" }
    }
  },

    exhaust_manifold_gp_stage_1 = {
    name = [[MANİFOLD]],
    description = [[Manifold, daha sağlam ve hafif malzemelerden yapılmış bir modelle değiştirilir.]],
    stage =1,
    staff_level =21,
    category = "exhaust",
    type =0,
    time =45,
    championship = "gp",
    effects = {
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00068" },
[ "bike/data/engine/MaxTorque" ] = { "-0.01728" },
[ "bike/data/body/Mass" ] = { "0.02700" }
    }
  },

    exhaust_manifold_gp_stage_2 = {
    name = [[MANİFOLD]],
    description = [[İkinci aşama geliştirmesi, daha verimli hava akışı için manifoldu motorun ateşleme sırasına göre ayarlar.]],
    stage =2,
    staff_level =23,
    category = "exhaust",
    type =0,
    time =60,
    championship = "gp",
    effects = {
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00055" },
[ "bike/data/engine/MaxTorque" ] = { "-0.01409" },
[ "bike/data/body/Mass" ] = { "0.02202" }
    }
  },

    exhaust_manifold_gp_stage_3 = {
    name = [[MANİFOLD]],
    description = [[Bu geliştirme, hava akışındaki basınç değişimlerinden yararlanarak gazları motorun içinden tam zamanında geçirmek için manifoldu daha da iyileştirir.]],
    stage =3,
    staff_level =25,
    category = "exhaust",
    type =0,
    time =75,
    championship = "gp",
    effects = {
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00034" },
[ "bike/data/engine/MaxTorque" ] = { "-0.00879" },
[ "bike/data/body/Mass" ] = { "0.01374" }
    }
  },

    exhaust_manifold_gp_stage_4 = {
    name = [[MANİFOLD]],
    description = [[Bu geliştirme, hava akışındaki basınç değişimlerinden yararlanarak gazları motorun içinden tam zamanında geçirmek için manifoldu daha da iyileştirir.]],
    stage =4,
    staff_level =27,
    category = "exhaust",
    type =0,
    time =90,
    championship = "gp",
    effects = {
[ "bike/data/gearbox/DifferentialRatio" ] = { "0.00000" },
[ "bike/data/engine/MaxTorque" ] = { "0.00000" },
[ "bike/data/body/Mass" ] = { "0.00000" }
    }
  },

  -- # EXHAUST - EXHAUST

    exhaust_exhaust_gp_stage_0 = {
    name = "",
    description = "",
    stage =0,
    staff_level =0,
    category = "exhaust",
    type =1,
    time =0,
    championship = "gp",
    effects = {
[ "bike/data/body/Mass" ] = { "0.03000" },
[ "bike/data/engine/MaxTorque" ] = { "-0.00600" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00020" },
[ "bike/data/lean/rate/TorqueMax" ] = { "-0.05000" },
[ "bike/data/lean/rate/Damping" ] = { "0.07500" }
    }
  },

    exhaust_exhaust_gp_stage_1 = {
    name = [[EGZOZ]],
    description = [[Motosiklete daha hafif ve sağlam yeni bir egzoz sistemi takılır.]],
    stage =1,
    staff_level =18,
    category = "exhaust",
    type =1,
    time =40,
    championship = "gp",
    effects = {
[ "bike/data/body/Mass" ] = { "0.02400" },
[ "bike/data/engine/MaxTorque" ] = { "-0.00480" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00016" },
[ "bike/data/lean/rate/TorqueMax" ] = { "-0.04000" },
[ "bike/data/lean/rate/Damping" ] = { "0.06000" }
    }
  },

    exhaust_exhaust_gp_stage_2 = {
    name = [[EGZOZ]],
    description = [[Egzozun hava akışında ve dayanıklılığında küçük iyileştirmeler.]],
    stage =2,
    staff_level =20,
    category = "exhaust",
    type =1,
    time =60,
    championship = "gp",
    effects = {
[ "bike/data/body/Mass" ] = { "0.01463" },
[ "bike/data/engine/MaxTorque" ] = { "-0.00293" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00010" },
[ "bike/data/lean/rate/TorqueMax" ] = { "-0.02438" },
[ "bike/data/lean/rate/Damping" ] = { "0.03657" }
    }
  },

    exhaust_exhaust_gp_stage_3 = {
    name = [[EGZOZ]],
    description = [[Motordan çıkan hava akışı iyileştirilir; böylece egzoz gazlarını dışarı atarken daha az enerji harcanır.]],
    stage =3,
    staff_level =22,
    category = "exhaust",
    type =1,
    time =80,
    championship = "gp",
    effects = {
[ "bike/data/body/Mass" ] = { "0.00000" },
[ "bike/data/engine/MaxTorque" ] = { "0.00000" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "0.00000" },
[ "bike/data/lean/rate/TorqueMax" ] = { "0.00000" },
[ "bike/data/lean/rate/Damping" ] = { "0.00000" }
    }
  },

  -- # EXHAUST - AIR

    exhaust_air_gp_stage_0 = {
    name = "",
    description = "",
    stage =0,
    staff_level =0,
    category = "exhaust",
    type =2,
    time =0,
    championship = "gp",
    effects = {
[ "bike/data/engine/MaxTorque" ] = { "-0.01200" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00050" }
    }
  },

    exhaust_air_gp_stage_1 = {
    name = [[HAVA FİLTRESİ]],
    description = [[Hava filtresi, kirin motora ulaşmasını engelleyen daha dayanıklı ve verimli bir modelle değiştirilir.]],
    stage =1,
    staff_level =19,
    category = "exhaust",
    type =2,
    time =60,
    championship = "gp",
    effects = {
[ "bike/data/engine/MaxTorque" ] = { "-0.00600" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00025" }
    }
  },

    exhaust_air_gp_stage_2 = {
    name = [[HAVA FİLTRESİ]],
    description = [[Hava filtresindeki ek iyileştirmeler hızlanmayı ve güç çıkışını artırır.]],
    stage =2,
    staff_level =25,
    category = "exhaust",
    type =2,
    time =80,
    championship = "gp",
    effects = {
[ "bike/data/engine/MaxTorque" ] = { "0.00000" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "0.00000" }
    }
  },

  -- # EXHAUST - OIL / FUEL

    exhaust_oilfuel_gp_stage_0 = {
    name = "",
    description = "",
    stage =0,
    staff_level =0,
    category = "exhaust",
    type =3,
    time =0,
    championship = "gp",
    effects = {
[ "bike/data/engine/MaxTorque" ] = { "-0.01680" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00070" }
    }
  },

    exhaust_oilfuel_gp_stage_1 = {
    name = [[YAĞ/YAKIT]],
    description = [[Motorda kullanılan yağın kalitesini yükselterek sürtünmeyi azaltır.]],
    stage =1,
    staff_level =15,
    category = "exhaust",
    type =3,
    time =20,
    championship = "gp",
    effects = {
[ "bike/data/engine/MaxTorque" ] = { "-0.01344" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00056" }
    }
  },

    exhaust_oilfuel_gp_stage_2 = {
    name = [[YAĞ/YAKIT]],
    description = [[Daha fazla enerji açığa çıkaran ve daha yüksek güç sağlayan, yüksek kaliteli ve oktanı artırılmış yakıt geliştirir.]],
    stage =2,
    staff_level =17,
    category = "exhaust",
    type =3,
    time =25,
    championship = "gp",
    effects = {
[ "bike/data/engine/MaxTorque" ] = { "-0.00819" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00034" }
    }
  },

    exhaust_oilfuel_gp_stage_3 = {
    name = [[YAĞ/YAKIT]],
    description = [[Motosikletin gereksiz yakıt ve dolayısıyla fazladan ağırlık taşımaması için kesin yakıt gereksinimleri araştırılır.]],
    stage =3,
    staff_level =20,
    category = "exhaust",
    type =3,
    time =30,
    championship = "gp",
    effects = {
[ "bike/data/engine/MaxTorque" ] = { "0.00000" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "0.00000" }
    }
  },

  -- # FRAME - CHASSIS

    frame_chassis_gp_stage_0 = {
    name = "",
    description = "",
    stage =0,
    staff_level =0,
    category = "frame",
    type =0,
    time =0,
    championship = "gp",
    effects = {
[ "bike/data/body/Mass" ] = { "0.06000" },
[ "bike/data/lean/rate/TorqueMax" ] = { "-0.40000" },
[ "bike/data/lean/rate/Damping" ] = { "0.30000" },
[ "bike/data/lean/angle/SteerBias" ] = { "0.00000" }
    }
  },

    frame_chassis_gp_stage_1 = {
    name = [[ŞASİ]],
    description = [[Şasi yapımı için hem sağlam hem hafif yeni malzemeler geliştirir.]],
    stage =1,
    staff_level =16,
    category = "frame",
    type =0,
    time =30,
    championship = "gp",
    effects = {
[ "bike/data/body/Mass" ] = { "0.04800" },
[ "bike/data/lean/rate/TorqueMax" ] = { "-0.32000" },
[ "bike/data/lean/rate/Damping" ] = { "0.24000" },
[ "bike/data/lean/angle/SteerBias" ] = { "0.00000" }
    }
  },

    frame_chassis_gp_stage_2 = {
    name = [[ŞASİ]],
    description = [[Ağırlık daha da azaltılarak hızlanma ve son hız önemli ölçüde iyileştirilir.]],
    stage =2,
    staff_level =22,
    category = "frame",
    type =0,
    time =50,
    championship = "gp",
    effects = {
[ "bike/data/body/Mass" ] = { "0.02926" },
[ "bike/data/lean/rate/TorqueMax" ] = { "-0.19504" },
[ "bike/data/lean/rate/Damping" ] = { "0.14628" },
[ "bike/data/lean/angle/SteerBias" ] = { "0.00000" }
    }
  },

    frame_chassis_gp_stage_3 = {
    name = [[ŞASİ]],
    description = [[Ağırlık sınırlarını aşmadan şasiyi güçlendirerek virajlarda daha yüksek denge sağlar.]],
    stage =3,
    staff_level =28,
    category = "frame",
    type =0,
    time =70,
    championship = "gp",
    effects = {
[ "bike/data/body/Mass" ] = { "0.00000" },
[ "bike/data/lean/rate/TorqueMax" ] = { "0.00000" },
[ "bike/data/lean/rate/Damping" ] = { "0.00000" },
[ "bike/data/lean/angle/SteerBias" ] = { "0.00000" }
    }
  },

  -- # FRAME - FAIRING

    frame_fairing_gp_stage_0 = {
    name = "",
    description = "",
    stage =0,
    staff_level =0,
    category = "frame",
    type =1,
    time =0,
    championship = "gp",
    effects = {
[ "bike/data/aerodynamic/BikeDragCoeff" ] = { "0.03400" },
[ "bike/data/aerodynamic/BikeReferenceArea" ] = { "0.03400" },
[ "bike/data/aerodynamic/RiderReferenceArea" ] = { "0.04000" }
    }
  },

    frame_fairing_gp_stage_1 = {
    name = [[GRENAJ]],
    description = [[Grenaj ve motosiklet biçimindeki temel iyileştirmeler, sürücü eğildiğinde havanın sırtı üzerinden daha düzgün akmasını sağlar.]],
    stage =1,
    staff_level =18,
    category = "frame",
    type =1,
    time =40,
    championship = "gp",
    effects = {
[ "bike/data/aerodynamic/BikeDragCoeff" ] = { "0.03060" },
[ "bike/data/aerodynamic/BikeReferenceArea" ] = { "0.03060" },
[ "bike/data/aerodynamic/RiderReferenceArea" ] = { "0.03600" }
    }
  },

    frame_fairing_gp_stage_2 = {
    name = [[GRENAJ]],
    description = [[Grenajdaki orta düzey değişiklikler, motosikletin önemli bölgelerindeki aerodinamik verimliliği artırır.]],
    stage =2,
    staff_level =21,
    category = "frame",
    type =1,
    time =60,
    championship = "gp",
    effects = {
[ "bike/data/aerodynamic/BikeDragCoeff" ] = { "0.02600" },
[ "bike/data/aerodynamic/BikeReferenceArea" ] = { "0.02600" },
[ "bike/data/aerodynamic/RiderReferenceArea" ] = { "0.03059" }
    }
  },

    frame_fairing_gp_stage_3 = {
    name = [[GRENAJ]],
    description = [[Bileşenlerin aşırı ısınmasını önlemek için ısı dağılımına odaklanan ek grenaj iyileştirmeleri.]],
    stage =3,
    staff_level =24,
    category = "frame",
    type =1,
    time =80,
    championship = "gp",
    effects = {
[ "bike/data/aerodynamic/BikeDragCoeff" ] = { "0.01978" },
[ "bike/data/aerodynamic/BikeReferenceArea" ] = { "0.01978" },
[ "bike/data/aerodynamic/RiderReferenceArea" ] = { "0.02327" }
    }
  },

    frame_fairing_gp_stage_4 = {
    name = [[GRENAJ]],
    description = [[Motosikletin aerodinamik verimliliğini daha da artırmak için rüzgâr tüneli araştırması yapılır.]],
    stage =4,
    staff_level =27,
    category = "frame",
    type =1,
    time =100,
    championship = "gp",
    effects = {
[ "bike/data/aerodynamic/BikeDragCoeff" ] = { "0.01137" },
[ "bike/data/aerodynamic/BikeReferenceArea" ] = { "0.01137" },
[ "bike/data/aerodynamic/RiderReferenceArea" ] = { "0.01338" }
    }
  },

    frame_fairing_gp_stage_5 = {
    name = [[GRENAJ]],
    description = [[Bu geliştirme, motosikletin diğer özelliklerini olumsuz etkilemeden grenajın hava direncini mümkün olan en düşük düzeye indirir.]],
    stage =5,
    staff_level =30,
    category = "frame",
    type =1,
    time =120,
    championship = "gp",
    effects = {
[ "bike/data/aerodynamic/BikeDragCoeff" ] = { "0.00000" },
[ "bike/data/aerodynamic/BikeReferenceArea" ] = { "0.00000" },
[ "bike/data/aerodynamic/RiderReferenceArea" ] = { "0.00000" }
    }
  },

  -- # FRAME - SWING ARM

  frame_swingarm_gp_stage_0 = {
    name = "",
    description = "",
    stage =0,
    staff_level =0,
    category = "frame",
    type =2,
    time =0,
    championship = "gp",
    effects = {
[ "bike/data/DrivingAids/RearBrakeControl" ] = { "-0.03000" },
[ "bike/data/DrivingAids/RearBrakeControlSetupMax" ] = { "-0.06000" },
[ "bike/data/DrivingAids/RearBrakeControlSetupMin" ] = { "0.00000" },
[ "bike/data/lean/rate/TorqueMax" ] = { "-0.15000" },
[ "bike/data/lean/rate/Damping" ] = { "0.15000" },
[ "bike/data/lean/angle/SteerBias" ] = { "0.00000" },
[ "bike/data/NormalLoadEQ/DampingRate" ] = { "-0.60000" },
[ "bike/data/suspensions/1/Stiffness" ] = { "-10.00000" },
[ "bike/data/suspensions/1/StiffnessSetupMax" ] = { "-10.00000" },
[ "bike/data/suspensions/1/StiffnessSetupMin" ] = { "-10.00000" },
[ "bike/data/suspensions/1/CompressionDamping" ] = { "-1.00000" },
[ "bike/data/suspensions/1/CompressionDampingSetupMax" ] = { "-1.00000" },
[ "bike/data/suspensions/1/CompressionDampingSetupMin" ] = { "-1.00000" },
[ "bike/data/suspensions/1/ReboundDamping" ] = { "-1.50000" },
[ "bike/data/suspensions/1/ReboundDampingSetupMin" ] = { "-1.50000" },
[ "bike/data/suspensions/1/ReboundDampingSetupMax" ] = { "-1.50000" }
    }
  },

    frame_swingarm_gp_stage_1 = {
    name = [[SALINCAK]],
    description = [[Salıncak, zincir torkunun oluşturduğu çok büyük baskı kuvvetlerine dayanmalıdır. Daha sağlam malzemeler kullanılması salıncağın dayanıklılığını artırır.]],
    stage =1,
    staff_level =19,
    category = "frame",
    type =2,
    time =30,
    championship = "gp",
    effects = {
[ "bike/data/DrivingAids/RearBrakeControl" ] = { "-0.02400" },
[ "bike/data/DrivingAids/RearBrakeControlSetupMax" ] = { "-0.04800" },
[ "bike/data/DrivingAids/RearBrakeControlSetupMin" ] = { "0.00000" },
[ "bike/data/lean/rate/TorqueMax" ] = { "-0.12000" },
[ "bike/data/lean/rate/Damping" ] = { "0.12000" },
[ "bike/data/lean/angle/SteerBias" ] = { "0.00000" },
[ "bike/data/NormalLoadEQ/DampingRate" ] = { "-0.48000" },
[ "bike/data/suspensions/1/Stiffness" ] = { "-8.00000" },
[ "bike/data/suspensions/1/StiffnessSetupMax" ] = { "-8.00000" },
[ "bike/data/suspensions/1/StiffnessSetupMin" ] = { "-8.00000" },
[ "bike/data/suspensions/1/CompressionDamping" ] = { "-0.80000" },
[ "bike/data/suspensions/1/CompressionDampingSetupMax" ] = { "-0.80000" },
[ "bike/data/suspensions/1/CompressionDampingSetupMin" ] = { "-0.80000" },
[ "bike/data/suspensions/1/ReboundDamping" ] = { "-1.20000" },
[ "bike/data/suspensions/1/ReboundDampingSetupMin" ] = { "-1.20000" },
[ "bike/data/suspensions/1/ReboundDampingSetupMax" ] = { "-1.20000" }
    }
  },

    frame_swingarm_gp_stage_2 = {
    name = [[SALINCAK]],
    description = [[Salıncak süspansiyonundaki iyileştirmeler, viraj çıkışında hızlanırken daha yüksek kontrol sağlar.]],
    stage =2,
    staff_level =22,
    category = "frame",
    type =2,
    time =45,
    championship = "gp",
    effects = {
[ "bike/data/DrivingAids/RearBrakeControl" ] = { "-0.01463" },
[ "bike/data/DrivingAids/RearBrakeControlSetupMax" ] = { "-0.02926" },
[ "bike/data/DrivingAids/RearBrakeControlSetupMin" ] = { "0.00000" },
[ "bike/data/lean/rate/TorqueMax" ] = { "-0.07314" },
[ "bike/data/lean/rate/Damping" ] = { "0.07314" },
[ "bike/data/lean/angle/SteerBias" ] = { "0.00000" },
[ "bike/data/NormalLoadEQ/DampingRate" ] = { "-0.29256" },
[ "bike/data/suspensions/1/Stiffness" ] = { "-4.87600" },
[ "bike/data/suspensions/1/StiffnessSetupMax" ] = { "-4.87600" },
[ "bike/data/suspensions/1/StiffnessSetupMin" ] = { "-4.87600" },
[ "bike/data/suspensions/1/CompressionDamping" ] = { "-0.48760" },
[ "bike/data/suspensions/1/CompressionDampingSetupMax" ] = { "-0.48760" },
[ "bike/data/suspensions/1/CompressionDampingSetupMin" ] = { "-0.48760" },
[ "bike/data/suspensions/1/ReboundDamping" ] = { "-0.73140" },
[ "bike/data/suspensions/1/ReboundDampingSetupMin" ] = { "-0.73140" },
[ "bike/data/suspensions/1/ReboundDampingSetupMax" ] = { "-0.73140" }
    }
  },

    frame_swingarm_gp_stage_3 = {
    name = [[SALINCAK]],
    description = [[Yüksek hızlı düzlüklerde dengeyi artıran ek salıncak iyileştirmeleri. Bu geliştirme, salıncağın tüm ayarlarını özelleştirmenizi sağlar.]],
    stage =3,
    staff_level =25,
    category = "frame",
    type =2,
    time =60,
    championship = "gp",
    effects = {
[ "bike/data/DrivingAids/RearBrakeControl" ] = { "0.00000" },
[ "bike/data/DrivingAids/RearBrakeControlSetupMax" ] = { "0.00000" },
[ "bike/data/DrivingAids/RearBrakeControlSetupMin" ] = { "0.00000" },
[ "bike/data/lean/rate/TorqueMax" ] = { "0.00000" },
[ "bike/data/lean/rate/Damping" ] = { "0.00000" },
[ "bike/data/lean/angle/SteerBias" ] = { "0.00000" },
[ "bike/data/NormalLoadEQ/DampingRate" ] = { "0.00000" },
[ "bike/data/suspensions/1/Stiffness" ] = { "0.00000" },
[ "bike/data/suspensions/1/StiffnessSetupMax" ] = { "0.00000" },
[ "bike/data/suspensions/1/StiffnessSetupMin" ] = { "0.00000" },
[ "bike/data/suspensions/1/CompressionDamping" ] = { "0.00000" },
[ "bike/data/suspensions/1/CompressionDampingSetupMax" ] = { "0.00000" },
[ "bike/data/suspensions/1/CompressionDampingSetupMin" ] = { "0.00000" },
[ "bike/data/suspensions/1/ReboundDamping" ] = { "0.00000" },
[ "bike/data/suspensions/1/ReboundDampingSetupMin" ] = { "0.00000" },
[ "bike/data/suspensions/1/ReboundDampingSetupMax" ] = { "0.00000" }
    }
  },

  -- # FRAME - SHOCK ABSORBERS

    frame_shock_absorbers_gp_stage_0 = {
    name = "",
    description = "",
    stage =0,
    staff_level =0,
    category = "frame",
    type =3,
    time =0,
    championship = "gp",
    effects = {
[ "bike/data/DrivingAids/FrontBrakeControl" ] = { "-0.15000" },
[ "bike/data/DrivingAids/FrontBrakeControlSetupMax" ] = { "-0.15000" },
[ "bike/data/DrivingAids/FrontBrakeControlSetupMin" ] = { "0.00000" },
[ "bike/data/lean/rate/TorqueMax" ] = { "-0.10000" },
[ "bike/data/lean/rate/Damping" ] = { "0.15000" },
[ "bike/data/lean/angle/SteerBias" ] = { "0.00000" },
[ "bike/data/NormalLoadEQ/DampingRate" ] = { "-2.10000" },
[ "bike/data/suspensions/0/Stiffness" ] = { "-20.00000" },
[ "bike/data/suspensions/0/StiffnessSetupMax" ] = { "-20.00000" },
[ "bike/data/suspensions/0/StiffnessSetupMin" ] = { "-20.00000" },
[ "bike/data/suspensions/0/CompressionDamping" ] = { "-2.00000" },
[ "bike/data/suspensions/0/CompressionDampingSetupMax" ] = { "-2.00000" },
[ "bike/data/suspensions/0/CompressionDampingSetupMin" ] = { "-2.00000" },
[ "bike/data/suspensions/0/ReboundDamping" ] = { "-3.00000" },
[ "bike/data/suspensions/0/ReboundDampingSetupMax" ] = { "-3.00000" },
[ "bike/data/suspensions/0/ReboundDampingSetupMin" ] = { "-3.00000" },
[ "bike/data/suspensions/1/Stiffness" ] = { "-10.00000" },
[ "bike/data/suspensions/1/StiffnessSetupMax" ] = { "-10.00000" },
[ "bike/data/suspensions/1/StiffnessSetupMin" ] = { "-10.00000" },
[ "bike/data/suspensions/1/CompressionDamping" ] = { "-1.00000" },
[ "bike/data/suspensions/1/CompressionDampingSetupMax" ] = { "-1.00000" },
[ "bike/data/suspensions/1/CompressionDampingSetupMin" ] = { "-1.00000" },
[ "bike/data/suspensions/1/ReboundDamping" ] = { "-1.50000" },
[ "bike/data/suspensions/1/ReboundDampingSetupMin" ] = { "-1.50000" },
[ "bike/data/suspensions/1/ReboundDampingSetupMax" ] = { "-1.50000" },
[ "bike/data/setup/unlock/Suspension" ] = { "-1" }
    }
  },

    frame_shock_absorbers_gp_stage_1 = {
    name = [[AMORTİSÖRLER]],
    description = [[Amortisörleri daha gelişmiş bir süspansiyon sistemiyle değiştirerek frenleme ve viraj dengesini artırır.]],
    stage =1,
    staff_level =17,
    category = "frame",
    type =3,
    time =50,
    championship = "gp",
    effects = {
[ "bike/data/DrivingAids/FrontBrakeControl" ] = { "-0.13500" },
[ "bike/data/DrivingAids/FrontBrakeControlSetupMax" ] = { "-0.13500" },
[ "bike/data/DrivingAids/FrontBrakeControlSetupMin" ] = { "0.00000" },
[ "bike/data/lean/rate/TorqueMax" ] = { "-0.09000" },
[ "bike/data/lean/rate/Damping" ] = { "0.13500" },
[ "bike/data/lean/angle/SteerBias" ] = { "0.00000" },
[ "bike/data/NormalLoadEQ/DampingRate" ] = { "-1.89000" },
[ "bike/data/suspensions/0/Stiffness" ] = { "-18.00000" },
[ "bike/data/suspensions/0/StiffnessSetupMax" ] = { "-18.00000" },
[ "bike/data/suspensions/0/StiffnessSetupMin" ] = { "-18.00000" },
[ "bike/data/suspensions/0/CompressionDamping" ] = { "-1.80000" },
[ "bike/data/suspensions/0/CompressionDampingSetupMax" ] = { "-1.80000" },
[ "bike/data/suspensions/0/CompressionDampingSetupMin" ] = { "-1.80000" },
[ "bike/data/suspensions/0/ReboundDamping" ] = { "-2.70000" },
[ "bike/data/suspensions/0/ReboundDampingSetupMax" ] = { "-2.70000" },
[ "bike/data/suspensions/0/ReboundDampingSetupMin" ] = { "-2.70000" },
[ "bike/data/suspensions/1/Stiffness" ] = { "-9.00000" },
[ "bike/data/suspensions/1/StiffnessSetupMax" ] = { "-9.00000" },
[ "bike/data/suspensions/1/StiffnessSetupMin" ] = { "-9.00000" },
[ "bike/data/suspensions/1/CompressionDamping" ] = { "-0.90000" },
[ "bike/data/suspensions/1/CompressionDampingSetupMax" ] = { "-0.90000" },
[ "bike/data/suspensions/1/CompressionDampingSetupMin" ] = { "-0.90000" },
[ "bike/data/suspensions/1/ReboundDamping" ] = { "-1.35000" },
[ "bike/data/suspensions/1/ReboundDampingSetupMin" ] = { "-1.35000" },
[ "bike/data/suspensions/1/ReboundDampingSetupMax" ] = { "-1.35000" },
[ "bike/data/setup/unlock/Suspension" ] = { "-1" }
    }
  },

  frame_shock_absorbers_gp_stage_2 = {
    name = [[AMORTİSÖRLER]],
    description = [[Mevcut süspansiyon sistemini daha iyi performans için yeniden ayarlar.]],
    stage =2,
    staff_level =18,
    category = "frame",
    type =3,
    time =55,
    championship = "gp",
    effects = {
[ "bike/data/DrivingAids/FrontBrakeControl" ] = { "-0.11471" },
[ "bike/data/DrivingAids/FrontBrakeControlSetupMax" ] = { "-0.11471" },
[ "bike/data/DrivingAids/FrontBrakeControlSetupMin" ] = { "0.00000" },
[ "bike/data/lean/rate/TorqueMax" ] = { "-0.07648" },
[ "bike/data/lean/rate/Damping" ] = { "0.11471" },
[ "bike/data/lean/angle/SteerBias" ] = { "0.00000" },
[ "bike/data/NormalLoadEQ/DampingRate" ] = { "-1.60598" },
[ "bike/data/suspensions/0/Stiffness" ] = { "-15.29500" },
[ "bike/data/suspensions/0/StiffnessSetupMax" ] = { "-15.29500" },
[ "bike/data/suspensions/0/StiffnessSetupMin" ] = { "-15.29500" },
[ "bike/data/suspensions/0/CompressionDamping" ] = { "-1.52950" },
[ "bike/data/suspensions/0/CompressionDampingSetupMax" ] = { "-1.52950" },
[ "bike/data/suspensions/0/CompressionDampingSetupMin" ] = { "-1.52950" },
[ "bike/data/suspensions/0/ReboundDamping" ] = { "-2.29425" },
[ "bike/data/suspensions/0/ReboundDampingSetupMax" ] = { "-2.29425" },
[ "bike/data/suspensions/0/ReboundDampingSetupMin" ] = { "-2.29425" },
[ "bike/data/suspensions/1/Stiffness" ] = { "-7.64750" },
[ "bike/data/suspensions/1/StiffnessSetupMax" ] = { "-7.64750" },
[ "bike/data/suspensions/1/StiffnessSetupMin" ] = { "-7.64750" },
[ "bike/data/suspensions/1/CompressionDamping" ] = { "-0.76475" },
[ "bike/data/suspensions/1/CompressionDampingSetupMax" ] = { "-0.76475" },
[ "bike/data/suspensions/1/CompressionDampingSetupMin" ] = { "-0.76475" },
[ "bike/data/suspensions/1/ReboundDamping" ] = { "-1.14713" },
[ "bike/data/suspensions/1/ReboundDampingSetupMin" ] = { "-1.14713" },
[ "bike/data/suspensions/1/ReboundDampingSetupMax" ] = { "-1.14713" },
[ "bike/data/setup/unlock/Suspension" ] = { "-1" }
    }
  },

    frame_shock_absorbers_gp_stage_3 = {
    name = [[AMORTİSÖRLER]],
    description = [[Bozuk yüzeylerin darbelerini ve sert frenlemenin oluşturduğu sıkışmayı daha etkili sönümleyen gelişmiş amortisörler.]],
    stage =3,
    staff_level =19,
    category = "frame",
    type =3,
    time =60,
    championship = "gp",
    effects = {
[ "bike/data/DrivingAids/FrontBrakeControl" ] = { "-0.08727" },
[ "bike/data/DrivingAids/FrontBrakeControlSetupMax" ] = { "-0.08727" },
[ "bike/data/DrivingAids/FrontBrakeControlSetupMin" ] = { "0.00000" },
[ "bike/data/lean/rate/TorqueMax" ] = { "-0.05818" },
[ "bike/data/lean/rate/Damping" ] = { "0.08727" },
[ "bike/data/lean/angle/SteerBias" ] = { "0.00000" },
[ "bike/data/NormalLoadEQ/DampingRate" ] = { "-1.22183" },
[ "bike/data/suspensions/0/Stiffness" ] = { "-11.63649" },
[ "bike/data/suspensions/0/StiffnessSetupMax" ] = { "-11.63649" },
[ "bike/data/suspensions/0/StiffnessSetupMin" ] = { "-11.63649" },
[ "bike/data/suspensions/0/CompressionDamping" ] = { "-1.16365" },
[ "bike/data/suspensions/0/CompressionDampingSetupMax" ] = { "-1.16365" },
[ "bike/data/suspensions/0/CompressionDampingSetupMin" ] = { "-1.16365" },
[ "bike/data/suspensions/0/ReboundDamping" ] = { "-1.74547" },
[ "bike/data/suspensions/0/ReboundDampingSetupMax" ] = { "-1.74547" },
[ "bike/data/suspensions/0/ReboundDampingSetupMin" ] = { "-1.74547" },
[ "bike/data/suspensions/1/Stiffness" ] = { "-5.81824" },
[ "bike/data/suspensions/1/StiffnessSetupMax" ] = { "-5.81824" },
[ "bike/data/suspensions/1/StiffnessSetupMin" ] = { "-5.81824" },
[ "bike/data/suspensions/1/CompressionDamping" ] = { "-0.58182" },
[ "bike/data/suspensions/1/CompressionDampingSetupMax" ] = { "-0.58182" },
[ "bike/data/suspensions/1/CompressionDampingSetupMin" ] = { "-0.58182" },
[ "bike/data/suspensions/1/ReboundDamping" ] = { "-0.87274" },
[ "bike/data/suspensions/1/ReboundDampingSetupMin" ] = { "-0.87274" },
[ "bike/data/suspensions/1/ReboundDampingSetupMax" ] = { "-0.87274" },
[ "bike/data/setup/unlock/Suspension" ] = { "-1" }
    }
  },

    frame_shock_absorbers_gp_stage_4 = {
    name = [[AMORTİSÖRLER]],
    description = [[Bu süspansiyon sistemi daha hassas ayarlar sunarak mühendislerinizin daha yüksek denge elde etmesini sağlar.]],
    stage =4,
    staff_level =20,
    category = "frame",
    type =3,
    time =65,
    championship = "gp",
    effects = {
[ "bike/data/DrivingAids/FrontBrakeControl" ] = { "-0.05016" },
[ "bike/data/DrivingAids/FrontBrakeControlSetupMax" ] = { "-0.05016" },
[ "bike/data/DrivingAids/FrontBrakeControlSetupMin" ] = { "0.00000" },
[ "bike/data/lean/rate/TorqueMax" ] = { "-0.03344" },
[ "bike/data/lean/rate/Damping" ] = { "0.05016" },
[ "bike/data/lean/angle/SteerBias" ] = { "0.00000" },
[ "bike/data/NormalLoadEQ/DampingRate" ] = { "-0.70228" },
[ "bike/data/suspensions/0/Stiffness" ] = { "-6.68835" },
[ "bike/data/suspensions/0/StiffnessSetupMax" ] = { "-6.68835" },
[ "bike/data/suspensions/0/StiffnessSetupMin" ] = { "-6.68835" },
[ "bike/data/suspensions/0/CompressionDamping" ] = { "-0.66883" },
[ "bike/data/suspensions/0/CompressionDampingSetupMax" ] = { "-0.66883" },
[ "bike/data/suspensions/0/CompressionDampingSetupMin" ] = { "-0.66883" },
[ "bike/data/suspensions/0/ReboundDamping" ] = { "-1.00325" },
[ "bike/data/suspensions/0/ReboundDampingSetupMax" ] = { "-1.00325" },
[ "bike/data/suspensions/0/ReboundDampingSetupMin" ] = { "-1.00325" },
[ "bike/data/suspensions/1/Stiffness" ] = { "-3.34417" },
[ "bike/data/suspensions/1/StiffnessSetupMax" ] = { "-3.34417" },
[ "bike/data/suspensions/1/StiffnessSetupMin" ] = { "-3.34417" },
[ "bike/data/suspensions/1/CompressionDamping" ] = { "-0.33442" },
[ "bike/data/suspensions/1/CompressionDampingSetupMax" ] = { "-0.33442" },
[ "bike/data/suspensions/1/CompressionDampingSetupMin" ] = { "-0.33442" },
[ "bike/data/suspensions/1/ReboundDamping" ] = { "-0.50163" },
[ "bike/data/suspensions/1/ReboundDampingSetupMin" ] = { "-0.50163" },
[ "bike/data/suspensions/1/ReboundDampingSetupMax" ] = { "-0.50163" },
[ "bike/data/setup/unlock/Suspension" ] = { "-1" }
    }
  },

    frame_shock_absorbers_gp_stage_5 = {
    name = [[AMORTİSÖRLER]],
    description = [[En yüksek performans için süspansiyon düzenini tamamlar. Bu geliştirme, süspansiyonun tüm ayarlarını özelleştirmenizi sağlar.]],
    stage =5,
    staff_level =21,
    category = "frame",
    type =3,
    time =70,
    championship = "gp",
    effects = {
[ "bike/data/DrivingAids/FrontBrakeControl" ] = { "0.00000" },
[ "bike/data/DrivingAids/FrontBrakeControlSetupMax" ] = { "0.00000" },
[ "bike/data/DrivingAids/FrontBrakeControlSetupMin" ] = { "0.00000" },
[ "bike/data/lean/rate/TorqueMax" ] = { "0.00000" },
[ "bike/data/lean/rate/Damping" ] = { "0.00000" },
[ "bike/data/lean/angle/SteerBias" ] = { "0.00000" },
[ "bike/data/NormalLoadEQ/DampingRate" ] = { "0.00000" },
[ "bike/data/suspensions/0/Stiffness" ] = { "0.00000" },
[ "bike/data/suspensions/0/StiffnessSetupMax" ] = { "0.00000" },
[ "bike/data/suspensions/0/StiffnessSetupMin" ] = { "0.00000" },
[ "bike/data/suspensions/0/CompressionDamping" ] = { "0.00000" },
[ "bike/data/suspensions/0/CompressionDampingSetupMax" ] = { "0.00000" },
[ "bike/data/suspensions/0/CompressionDampingSetupMin" ] = { "0.00000" },
[ "bike/data/suspensions/0/ReboundDamping" ] = { "0.00000" },
[ "bike/data/suspensions/0/ReboundDampingSetupMax" ] = { "0.00000" },
[ "bike/data/suspensions/0/ReboundDampingSetupMin" ] = { "0.00000" },
[ "bike/data/suspensions/1/Stiffness" ] = { "0.00000" },
[ "bike/data/suspensions/1/StiffnessSetupMax" ] = { "0.00000" },
[ "bike/data/suspensions/1/StiffnessSetupMin" ] = { "0.00000" },
[ "bike/data/suspensions/1/CompressionDamping" ] = { "0.00000" },
[ "bike/data/suspensions/1/CompressionDampingSetupMax" ] = { "0.00000" },
[ "bike/data/suspensions/1/CompressionDampingSetupMin" ] = { "0.00000" },
[ "bike/data/suspensions/1/ReboundDamping" ] = { "0.00000" },
[ "bike/data/suspensions/1/ReboundDampingSetupMin" ] = { "0.00000" },
[ "bike/data/suspensions/1/ReboundDampingSetupMax" ] = { "0.00000" },
[ "bike/data/setup/unlock/Suspension" ] = { "0" }
    }
  },

  -- # GEARBOX - CLUTCH

    gearbox_clutch_gp_stage_0 = {
    name = "",
    description = "",
    stage =0,
    staff_level =0,
    category = "gearbox",
    type =0,
    time =0,
    championship = "gp",
    effects = {
[ "bike/data/body/Mass" ] = { "0.01500" },
[ "bike/data/engine/MaxCompressionTorque" ] = { "-0.10000" },
[ "bike/data/gearbox/ClutchingTime" ] = { "0.08000" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00025" }
    }
  },

    gearbox_clutch_gp_stage_1 = {
    name = [[DEBRİYAJ]],
    description = [[Standart debriyajı kaydırmalı debriyaj mekanizmasıyla değiştirerek vites küçültürken motosikleti çok daha dengeli hâle getirir.]],
    stage =1,
    staff_level =19,
    category = "gearbox",
    type =0,
    time =55,
    championship = "gp",
    effects = {
[ "bike/data/body/Mass" ] = { "0.01350" },
[ "bike/data/engine/MaxCompressionTorque" ] = { "-0.09000" },
[ "bike/data/gearbox/ClutchingTime" ] = { "0.07200" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00023" }
    }
  },

    gearbox_clutch_gp_stage_2 = {
    name = [[DEBRİYAJ]],
    description = [[Motor frenine ve daha hızlı vites değişimlerine daha iyi dayanan, güçlendirilmiş hafif debriyaj plakaları.]],
    stage =2,
    staff_level =20,
    category = "gearbox",
    type =0,
    time =70,
    championship = "gp",
    effects = {
[ "bike/data/body/Mass" ] = { "0.01101" },
[ "bike/data/engine/MaxCompressionTorque" ] = { "-0.07339" },
[ "bike/data/gearbox/ClutchingTime" ] = { "0.05871" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00018" }
    }
  },

    gearbox_clutch_gp_stage_3 = {
    name = [[DEBRİYAJ]],
    description = [[Debriyajdaki ısıyı daha etkili dağıtarak daha da hızlı vites değiştirmeyi sağlayan teknoloji.]],
    stage =3,
    staff_level =21,
    category = "gearbox",
    type =0,
    time =85,
    championship = "gp",
    effects = {
[ "bike/data/body/Mass" ] = { "0.00687" },
[ "bike/data/engine/MaxCompressionTorque" ] = { "-0.04580" },
[ "bike/data/gearbox/ClutchingTime" ] = { "0.03664" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00011" }
    }
  },

    gearbox_clutch_gp_stage_4 = {
    name = [[DEBRİYAJ]],
    description = [[Hassas debriyaj ayarı, vites değiştirerek hızlanırken sarsıntıyı ve debriyajın kaçırmasını büyük ölçüde azaltarak dengeyi artırır.]],
    stage =4,
    staff_level =22,
    category = "gearbox",
    type =0,
    time =100,
    championship = "gp",
    effects = {
[ "bike/data/body/Mass" ] = { "0.00000" },
[ "bike/data/engine/MaxCompressionTorque" ] = { "0.00000" },
[ "bike/data/gearbox/ClutchingTime" ] = { "0.00000" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "0.00000" }
    }
  },

  -- # GEARBOX - RATIOS

  gearbox_ratios_gp_stage_0 = {
    name = "",
    description = "",
    stage =0,
    staff_level =0,
    category = "gearbox",
    type =1,
    time =0,
    championship = "gp",
    effects = {
[ "bike/data/engine/MaxTorque" ] = { "-0.00600" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00025" },
[ "bike/data/setup/unlock/Gearbox" ] = { "-1" }
    }
  },

    gearbox_ratios_gp_stage_1 = {
    name = [[VİTES ORANLARI]],
    description = [[Daha iyi hızlanma sağlayan küçük bir şanzıman iyileştirmesi.]],
    stage =1,
    staff_level =16,
    category = "gearbox",
    type =1,
    time =50,
    championship = "gp",
    effects = {
[ "bike/data/engine/MaxTorque" ] = { "-0.00540" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00023" },
[ "bike/data/setup/unlock/Gearbox" ] = { "-1" }
    }
  },

    gearbox_ratios_gp_stage_2 = {
    name = [[VİTES ORANLARI]],
    description = [[Hızlanmayı daha da artırmak için motosiklete en uygun vites düzeni araştırılır.]],
    stage =2,
    staff_level =17,
    category = "gearbox",
    type =1,
    time =60,
    championship = "gp",
    effects = {
[ "bike/data/engine/MaxTorque" ] = { "-0.00459" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00019" },
[ "bike/data/setup/unlock/Gearbox" ] = { "-1" }
    }
  },

    gearbox_ratios_gp_stage_3 = {
    name = [[VİTES ORANLARI]],
    description = [[Motorun yüksek torkuna ve daha sert vites değişimlerine dayanmak için çeşitli şanzıman parçaları daha hafif ve sağlam malzemelerle değiştirilir.]],
    stage =3,
    staff_level =18,
    category = "gearbox",
    type =1,
    time =70,
    championship = "gp",
    effects = {
[ "bike/data/engine/MaxTorque" ] = { "-0.00349" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00015" },
[ "bike/data/setup/unlock/Gearbox" ] = { "-1" }
    }
  },

    gearbox_ratios_gp_stage_4 = {
    name = [[VİTES ORANLARI]],
    description = [[Tüm devir aralığında güç eğrisinden tam yararlanarak hızlanmayı önemli ölçüde artıran kapsamlı bir şanzıman yenilemesi.]],
    stage =4,
    staff_level =19,
    category = "gearbox",
    type =1,
    time =80,
    championship = "gp",
    effects = {
[ "bike/data/engine/MaxTorque" ] = { "-0.00201" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "-0.00008" },
[ "bike/data/setup/unlock/Gearbox" ] = { "-1" }
    }
  },

    gearbox_ratios_gp_stage_5 = {
    name = [[VİTES ORANLARI]],
    description = [[Motordan yarış için en iyi hızlanmayı almak üzere vites düzenini tamamlar. Bu geliştirme, vites oranlarının tüm ayarlarını özelleştirmenizi sağlar.]],
    stage =5,
    staff_level =20,
    category = "gearbox",
    type =1,
    time =90,
    championship = "gp",
    effects = {
[ "bike/data/engine/MaxTorque" ] = { "0.00000" },
[ "bike/data/gearbox/DifferentialRatio" ] = { "0.00000" },
[ "bike/data/setup/unlock/Gearbox" ] = { "0" }
    }
  },
}
