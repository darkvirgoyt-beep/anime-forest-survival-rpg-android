#include "ForestSliceMobileHUD.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"
#include "ForestSliceCharacter.h"
#include "ForestSliceCombatComponent.h"
#include "ForestSliceGraphicsSettingsSubsystem.h"
#include "ForestSliceMobPresentationComponent.h"
#include "ForestSliceWeaponComponent.h"
#include "Input/Reply.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SWidget.h"

FVector2D UForestSliceVirtualJoystick::CalculateValue(const FGeometry& InGeometry, const FVector2D& ScreenPosition) const
{
    const FVector2D LocalPosition = InGeometry.AbsoluteToLocal(ScreenPosition);
    const FVector2D Center = InGeometry.GetLocalSize() * 0.5f;
    const FVector2D Radius = FVector2D(
        FMath::Max(1.0f, InGeometry.GetLocalSize().X * 0.5f),
        FMath::Max(1.0f, InGeometry.GetLocalSize().Y * 0.5f));
    const FVector2D Offset(
        (LocalPosition.X - Center.X) / Radius.X,
        (LocalPosition.Y - Center.Y) / Radius.Y);
    return Offset.GetClampedToMaxSize(1.0f);
}

void UForestSliceVirtualJoystick::EmitValue(const FVector2D& Value)
{
    StickValue = Value.GetClampedToMaxSize(1.0f);
    ValueChanged.Broadcast(StickValue);
    Invalidate(EInvalidateWidgetReason::Paint);
}

FReply UForestSliceVirtualJoystick::NativeOnTouchStarted(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
    if (ActivePointerIndex != INDEX_NONE) return FReply::Unhandled();
    ActivePointerIndex = InGestureEvent.GetPointerIndex();
    EmitValue(CalculateValue(InGeometry, InGestureEvent.GetScreenSpacePosition()));
    return FReply::Handled().CaptureMouse(TakeWidget());
}

FReply UForestSliceVirtualJoystick::NativeOnTouchMoved(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
    if (ActivePointerIndex != InGestureEvent.GetPointerIndex()) return FReply::Unhandled();
    EmitValue(CalculateValue(InGeometry, InGestureEvent.GetScreenSpacePosition()));
    return FReply::Handled();
}

FReply UForestSliceVirtualJoystick::NativeOnTouchEnded(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
    if (ActivePointerIndex != InGestureEvent.GetPointerIndex()) return FReply::Unhandled();
    ActivePointerIndex = INDEX_NONE;
    EmitValue(FVector2D::ZeroVector);
    return FReply::Handled().ReleaseMouseCapture();
}

int32 UForestSliceVirtualJoystick::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry,
    const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId,
    const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
    const FSlateBrush* Brush = FCoreStyle::Get().GetBrush("WhiteBrush");
    const FPaintGeometry PaintGeometry = AllottedGeometry.ToPaintGeometry();
    FSlateDrawElement::MakeBox(
        OutDrawElements,
        LayerId,
        PaintGeometry,
        Brush,
        ESlateDrawEffect::None,
        FLinearColor(0.03f, 0.12f, 0.15f, 0.62f));

    const FVector2D Size = AllottedGeometry.GetLocalSize();
    const FVector2D Center = Size * 0.5f;
    const FVector2D KnobOffset = FVector2D(StickValue.X * Size.X * 0.30f, StickValue.Y * Size.Y * 0.30f);
    const FVector2D KnobSize = Size * 0.28f;
    const FVector2D KnobPosition = Center + KnobOffset - KnobSize * 0.5f;
    const FPaintGeometry KnobGeometry = AllottedGeometry.ToPaintGeometry(KnobSize, FSlateLayoutTransform(1.0f, KnobPosition));
    FSlateDrawElement::MakeBox(
        OutDrawElements,
        LayerId + 1,
        KnobGeometry,
        Brush,
        ESlateDrawEffect::None,
        FLinearColor(0.86f, 0.70f, 0.34f, 0.92f));
    return LayerId + 2;
}

void UForestSliceMobileHUD::NativeConstruct()
{
    Super::NativeConstruct();

    UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    GraphicsSettings = GameInstance ? GameInstance->GetSubsystem<UForestSliceGraphicsSettingsSubsystem>() : nullptr;
    if (!WidgetTree) return;
    if (!WidgetTree->RootWidget)
    {
        BuildRuntimeControlSurface();
    }
    RefreshGraphicsQualityLabel();
}

void UForestSliceMobileHUD::NativeDestruct()
{
    GraphicsSettings = nullptr;
    Super::NativeDestruct();
}

void UForestSliceMobileHUD::BuildRuntimeControlSurface()
{
    RuntimeCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("MobileControlCanvas"));
    WidgetTree->RootWidget = RuntimeCanvas;

    UForestSliceVirtualJoystick* MoveStick = WidgetTree->ConstructWidget<UForestSliceVirtualJoystick>(
        UForestSliceVirtualJoystick::StaticClass(), TEXT("MoveJoystick"));
    MoveStick->ValueChanged.AddDynamic(this, &UForestSliceMobileHUD::JoystickChanged);
    if (UCanvasPanelSlot* Slot = RuntimeCanvas->AddChildToCanvas(MoveStick))
    {
        Slot->SetAnchors(FAnchors(0.0f, 1.0f, 0.0f, 1.0f));
        Slot->SetPosition(FVector2D(28.0f, -238.0f));
        Slot->SetSize(FVector2D(210.0f, 210.0f));
        Slot->SetZOrder(10);
    }

    UForestSliceVirtualJoystick* LookStick = WidgetTree->ConstructWidget<UForestSliceVirtualJoystick>(
        UForestSliceVirtualJoystick::StaticClass(), TEXT("LookJoystick"));
    LookStick->ValueChanged.AddDynamic(this, &UForestSliceMobileHUD::LookPadChanged);
    if (UCanvasPanelSlot* Slot = RuntimeCanvas->AddChildToCanvas(LookStick))
    {
        Slot->SetAnchors(FAnchors(1.0f, 1.0f, 1.0f, 1.0f));
        Slot->SetAlignment(FVector2D(1.0f, 1.0f));
        Slot->SetPosition(FVector2D(-28.0f, -238.0f));
        Slot->SetSize(FVector2D(210.0f, 210.0f));
        Slot->SetZOrder(10);
    }

    UButton* SettingsButton = AddRuntimeButton(FText::FromString(TEXT("SETTINGS")), FVector2D(-190.0f, 24.0f), FVector2D(160.0f, 54.0f));
    if (SettingsButton)
    {
        if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(SettingsButton->Slot))
        {
            Slot->SetAnchors(FAnchors(1.0f, 0.0f, 1.0f, 0.0f));
            Slot->SetAlignment(FVector2D(1.0f, 0.0f));
            Slot->SetPosition(FVector2D(-190.0f, 24.0f));
            Slot->SetZOrder(30);
        }
        SettingsButton->OnClicked.AddDynamic(this, &UForestSliceMobileHUD::ToggleSettingsPressed);
    }

    const TArray<TPair<FString, FVector2D>> Actions = {
        {TEXT("ATTACK"), FVector2D(-28.0f, -330.0f)},
        {TEXT("HEAVY"), FVector2D(-154.0f, -330.0f)},
        {TEXT("JUMP"), FVector2D(-280.0f, -330.0f)},
        {TEXT("DODGE"), FVector2D(-406.0f, -330.0f)},
        {TEXT("SPRINT"), FVector2D(-28.0f, -394.0f)},
        {TEXT("GATHER"), FVector2D(-154.0f, -394.0f)}
    };
    for (const TPair<FString, FVector2D>& Action : Actions)
    {
        UButton* Button = AddRuntimeButton(FText::FromString(Action.Key), Action.Value, FVector2D(112.0f, 52.0f));
        if (!Button) continue;
        if (Action.Key == TEXT("ATTACK")) Button->OnClicked.AddDynamic(this, &UForestSliceMobileHUD::AttackPressed);
        else if (Action.Key == TEXT("HEAVY")) Button->OnClicked.AddDynamic(this, &UForestSliceMobileHUD::HeavyAttackPressed);
        else if (Action.Key == TEXT("JUMP")) Button->OnClicked.AddDynamic(this, &UForestSliceMobileHUD::JumpPressed);
        else if (Action.Key == TEXT("DODGE")) Button->OnClicked.AddDynamic(this, &UForestSliceMobileHUD::DodgePressed);
        else if (Action.Key == TEXT("SPRINT"))
        {
            Button->OnPressed.AddDynamic(this, &UForestSliceMobileHUD::SprintPressed);
            Button->OnReleased.AddDynamic(this, &UForestSliceMobileHUD::SprintReleased);
        }
        else if (Action.Key == TEXT("GATHER")) Button->OnClicked.AddDynamic(this, &UForestSliceMobileHUD::GatherPressed);
    }

    SettingsPanel = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("GraphicsSettingsPanel"));
    if (UCanvasPanelSlot* PanelSlot = RuntimeCanvas->AddChildToCanvas(SettingsPanel))
    {
        PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
        PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
        PanelSlot->SetPosition(FVector2D::ZeroVector);
        PanelSlot->SetSize(FVector2D(470.0f, 300.0f));
        PanelSlot->SetZOrder(50);
    }

    UBorder* SettingsBackground = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("GraphicsSettingsBackground"));
    SettingsBackground->SetBrushColor(FLinearColor(0.02f, 0.05f, 0.07f, 0.96f));
    SettingsPanel->AddChild(SettingsBackground);
    if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(SettingsBackground->Slot))
    {
        Slot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
        Slot->SetOffsets(FMargin(0.0f));
    }

    UTextBlock* SettingsTitle = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("GraphicsSettingsTitle"));
    SettingsTitle->SetText(FText::FromString(TEXT("GRAPHICS SETTINGS")));
    SettingsTitle->SetColorAndOpacity(FSlateColor(FLinearColor(0.91f, 0.75f, 0.38f, 1.0f)));
    SettingsPanel->AddChild(SettingsTitle);
    if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(SettingsTitle->Slot))
    {
        Slot->SetPosition(FVector2D(28.0f, 22.0f));
        Slot->SetSize(FVector2D(300.0f, 34.0f));
    }

    GraphicsQualityLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("GraphicsQualityLabel"));
    GraphicsQualityLabel->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    SettingsPanel->AddChild(GraphicsQualityLabel);
    if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(GraphicsQualityLabel->Slot))
    {
        Slot->SetPosition(FVector2D(28.0f, 82.0f));
        Slot->SetSize(FVector2D(410.0f, 34.0f));
    }

    USlider* QualitySlider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass(), TEXT("GraphicsQualitySlider"));
    QualitySlider->SetMinValue(0.0f);
    QualitySlider->SetMaxValue(1.0f);
    QualitySlider->SetValue(GraphicsSettings ? GraphicsSettings->GetGraphicsQuality() / 4.0f : 0.75f);
    QualitySlider->OnValueChanged.AddDynamic(this, &UForestSliceMobileHUD::GraphicsQualityChanged);
    SettingsPanel->AddChild(QualitySlider);
    if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(QualitySlider->Slot))
    {
        Slot->SetPosition(FVector2D(28.0f, 132.0f));
        Slot->SetSize(FVector2D(410.0f, 42.0f));
    }

    UButton* CloseSettings = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CloseGraphicsSettings"));
    if (CloseSettings)
    {
        UTextBlock* CloseLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CloseGraphicsSettingsLabel"));
        CloseLabel->SetText(FText::FromString(TEXT("CLOSE")));
        CloseLabel->SetJustification(ETextJustify::Center);
        CloseLabel->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.89f, 0.70f, 1.0f)));
        CloseSettings->AddChild(CloseLabel);
        CloseSettings->OnClicked.AddDynamic(this, &UForestSliceMobileHUD::CloseSettingsPressed);
        SettingsPanel->AddChild(CloseSettings);
        if (UCanvasPanelSlot* Slot = Cast<UCanvasPanelSlot>(CloseSettings->Slot))
        {
            Slot->SetPosition(FVector2D(290.0f, 224.0f));
            Slot->SetSize(FVector2D(150.0f, 50.0f));
        }
    }
    SettingsPanel->SetVisibility(ESlateVisibility::Collapsed);
}

UButton* UForestSliceMobileHUD::AddRuntimeButton(const FText& Label, const FVector2D& Position, const FVector2D& Size)
{
    if (!RuntimeCanvas) return nullptr;
    UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
    UTextBlock* Text = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
    Text->SetText(Label);
    Text->SetJustification(ETextJustify::Center);
    Text->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.89f, 0.70f, 1.0f)));
    Button->AddChild(Text);
    if (UCanvasPanelSlot* Slot = RuntimeCanvas->AddChildToCanvas(Button))
    {
        Slot->SetAnchors(FAnchors(1.0f, 1.0f, 1.0f, 1.0f));
        Slot->SetAlignment(FVector2D(1.0f, 1.0f));
        Slot->SetPosition(Position);
        Slot->SetSize(Size);
        Slot->SetZOrder(20);
    }
    return Button;
}

void UForestSliceMobileHUD::SetControlledCharacter(AForestSliceCharacter* InCharacter)
{
    ControlledCharacter = InCharacter;
    if (InCharacter) {
        bGyroSupported = InCharacter->HasGyroscopeSupport();
        if (!bGyroSupported) bGyroEnabled = false;
    }
    RefreshGyroVisualState();
}

void UForestSliceMobileHUD::JoystickChanged(FVector2D Value)
{
    if (ControlledCharacter.IsValid()) ControlledCharacter->SetVirtualMove(Value.GetClampedToMaxSize(1.0f));
}

void UForestSliceMobileHUD::LookPadChanged(FVector2D Value)
{
    if (ControlledCharacter.IsValid()) ControlledCharacter->SetVirtualLook(Value);
}

void UForestSliceMobileHUD::SprintPressed()
{
    if (ControlledCharacter.IsValid()) ControlledCharacter->SetVirtualSprintHeld(true);
}

void UForestSliceMobileHUD::SprintReleased()
{
    if (ControlledCharacter.IsValid()) ControlledCharacter->SetVirtualSprintHeld(false);
}

void UForestSliceMobileHUD::AttackPressed()
{
    if (ControlledCharacter.IsValid()) ControlledCharacter->TriggerVirtualLightAttack();
}

void UForestSliceMobileHUD::HeavyAttackPressed()
{
    if (ControlledCharacter.IsValid()) {
        if (UForestSliceCombatComponent* Combat = ControlledCharacter->FindComponentByClass<UForestSliceCombatComponent>()) {
            Combat->RequestHeavyAttack();
        }
    }
}

void UForestSliceMobileHUD::JumpPressed()
{
    if (ControlledCharacter.IsValid()) ControlledCharacter->TriggerVirtualJump();
}

void UForestSliceMobileHUD::DodgePressed()
{
    if (ControlledCharacter.IsValid()) ControlledCharacter->TriggerVirtualDodge();
}

void UForestSliceMobileHUD::SelectShovel()
{
    if (ControlledCharacter.IsValid()) ControlledCharacter->SetActiveGroundTool(EForestSliceTool::Shovel);
}

void UForestSliceMobileHUD::SelectPickaxe()
{
    if (ControlledCharacter.IsValid()) ControlledCharacter->SetActiveGroundTool(EForestSliceTool::Pickaxe);
}

void UForestSliceMobileHUD::PlanGroundPressed()
{
    if (ControlledCharacter.IsValid()) ControlledCharacter->TriggerVirtualPlanGround();
}

void UForestSliceMobileHUD::CreateFarmContourPressed()
{
    if (ControlledCharacter.IsValid()) ControlledCharacter->TriggerVirtualCreateFarmContour();
}

void UForestSliceMobileHUD::DigPressed()
{
    if (ControlledCharacter.IsValid()) ControlledCharacter->TriggerVirtualDig();
}

void UForestSliceMobileHUD::PlantSeedPressed()
{
    if (ControlledCharacter.IsValid()) ControlledCharacter->TriggerVirtualPlantSeed();
}

void UForestSliceMobileHUD::GatherPressed()
{
    if (ControlledCharacter.IsValid()) ControlledCharacter->TriggerVirtualCollect();
}

void UForestSliceMobileHUD::WeaponSwitchPressed(int32 SlotIndex)
{
    if (ControlledCharacter.IsValid()) {
        if (UForestSliceWeaponComponent* Weapons = ControlledCharacter->FindComponentByClass<UForestSliceWeaponComponent>()) {
            Weapons->RequestSwitchToSlot(SlotIndex);
        }
    }
}

void UForestSliceMobileHUD::SetGyroToggle(bool bEnabled)
{
    bGyroEnabled = bGyroSupported && bEnabled;
    if (ControlledCharacter.IsValid()) ControlledCharacter->SetGyroEnabled(bGyroEnabled);
    RefreshGyroVisualState();
}

void UForestSliceMobileHUD::SetGyroSensorSupport(bool bSupported)
{
    bGyroSupported = bSupported;
    if (!bGyroSupported) bGyroEnabled = false;
    if (ControlledCharacter.IsValid()) ControlledCharacter->SetDeviceGyroscopeSupport(bSupported);
    RefreshGyroVisualState();
}

void UForestSliceMobileHUD::PushGyroSample(float RotationX, float RotationY, float Sensitivity)
{
    if (!bGyroSupported || !bGyroEnabled || !ControlledCharacter.IsValid()) return;
    ControlledCharacter->ApplyGyroInput(RotationX, RotationY, Sensitivity);
}

void UForestSliceMobileHUD::ToggleSettingsPressed()
{
    if (!SettingsPanel) return;
    const bool bOpen = SettingsPanel->GetVisibility() != ESlateVisibility::Visible;
    SettingsPanel->SetVisibility(bOpen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    RefreshGraphicsQualityLabel();
}

void UForestSliceMobileHUD::CloseSettingsPressed()
{
    if (SettingsPanel) SettingsPanel->SetVisibility(ESlateVisibility::Collapsed);
}

void UForestSliceMobileHUD::GraphicsQualityChanged(float NormalizedValue)
{
    if (GraphicsSettings)
    {
        GraphicsSettings->SetGraphicsQuality(FMath::RoundToInt(FMath::Clamp(NormalizedValue, 0.0f, 1.0f) * 4.0f));
    }
    RefreshGraphicsQualityLabel();
}

FText UForestSliceMobileHUD::GetGraphicsQualityLabel() const
{
    return GraphicsSettings ? GraphicsSettings->GetGraphicsQualityLabel() : FText::FromString(TEXT("UNKNOWN"));
}

void UForestSliceMobileHUD::RefreshGraphicsQualityLabel()
{
    if (!GraphicsQualityLabel) return;
    GraphicsQualityLabel->SetText(FText::Format(
        FText::FromString(TEXT("QUALITY  •  {0}")),
        GetGraphicsQualityLabel()));
}

void UForestSliceMobileHUD::SetFocusedMob(UForestSliceMobPresentationComponent* InMob)
{
    FocusedMob = InMob;
}

void UForestSliceMobileHUD::ClearFocusedMob()
{
    FocusedMob.Reset();
}

float UForestSliceMobileHUD::GetFocusedMobHealthRatio() const
{
    return FocusedMob.IsValid() ? FocusedMob->GetHealthRatio() : 0.0f;
}

FName UForestSliceMobileHUD::GetFocusedMobDisplayName() const
{
    return FocusedMob.IsValid() ? FocusedMob->GetDisplayName() : NAME_None;
}

int32 UForestSliceMobileHUD::GetFocusedMobLevel() const
{
    return FocusedMob.IsValid() ? FocusedMob->GetLevel() : 0;
}

bool UForestSliceMobileHUD::IsFocusedMobBoss() const
{
    return FocusedMob.IsValid() && FocusedMob->IsBoss();
}

bool UForestSliceMobileHUD::IsFocusedMobBaseAffiliated() const
{
    return FocusedMob.IsValid() && FocusedMob->IsBaseAffiliated();
}

FName UForestSliceMobileHUD::GetFocusedMobBaseId() const
{
    return FocusedMob.IsValid() ? FocusedMob->GetBaseId() : NAME_None;
}
