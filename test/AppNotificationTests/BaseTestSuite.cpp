﻿// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "pch.h"

#include "AppNotification-Test-Constants.h"
#include "AppNotificationHelpers.h"
#include "BaseTestSuite.h"

using namespace WEX::Common;
using namespace WEX::Logging;
using namespace WEX::TestExecution;

using namespace winrt::Windows::ApplicationModel::Activation;
using namespace winrt::Windows::ApplicationModel::Background;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Foundation::Collections;
using namespace winrt::Windows::Management::Deployment;
using namespace winrt::Windows::Storage;
using namespace winrt::Windows::System;
using namespace winrt::Microsoft::Windows::AppNotifications;
using namespace AppNotificationHelpers;

void BaseTestSuite::ClassSetup()
{
    ::Test::Bootstrap::SetupPackages();
}

void BaseTestSuite::ClassCleanup()
{
    ::Test::Bootstrap::CleanupPackages();
}

void BaseTestSuite::MethodSetup()
{
    ::Test::Bootstrap::SetupBootstrap();

    bool isSelfContained{};
    VERIFY_SUCCEEDED(TestData::TryGetValue(L"SelfContained", isSelfContained));

    if (!isSelfContained)
    {
        ::WindowsAppRuntime::VersionInfo::TestInitialize(::Test::Bootstrap::TP::WindowsAppRuntimeFramework::c_PackageFamilyName);
        VERIFY_IS_FALSE(::WindowsAppRuntime::SelfContained::IsSelfContained());
    }
    else
    {
        ::WindowsAppRuntime::VersionInfo::TestInitialize(L"I_don't_exist_package!");
        VERIFY_IS_TRUE(::WindowsAppRuntime::SelfContained::IsSelfContained());
    }
    EnsureNoActiveToasts();
}

void BaseTestSuite::MethodCleanup()
{
    if (!m_unregisteredFully)
    {
        UnregisterAllWithAppNotificationManager();
    }

    ::WindowsAppRuntime::VersionInfo::TestShutdown();
    ::Test::Bootstrap::CleanupBootstrap();
}

void BaseTestSuite::RegisterWithAppNotificationManager()
{
    AppNotificationManager::Default().Register();
    m_unregisteredFully = false;
}

void BaseTestSuite::UnregisterAllWithAppNotificationManager()
{
    AppNotificationManager::Default().UnregisterAll();
    m_unregisteredFully = true;
}

void BaseTestSuite::VerifyRegisterActivatorandUnregisterActivator()
{
    RegisterWithAppNotificationManager();
    AppNotificationManager::Default().Unregister();
}

void BaseTestSuite::VerifyFailedMultipleRegister()
{
    RegisterWithAppNotificationManager();
    VERIFY_THROWS_HR(AppNotificationManager::Default().Register(), E_INVALIDARG);
}

void BaseTestSuite::VerifyUnregisterAll()
{
    RegisterWithAppNotificationManager();
    UnregisterAllWithAppNotificationManager();
}

void BaseTestSuite::VerifyUnregisterTwice()
{
    RegisterWithAppNotificationManager();
    AppNotificationManager::Default().Unregister();
    VERIFY_THROWS_HR(AppNotificationManager::Default().Unregister(), E_NOT_SET);
}
void BaseTestSuite::VerifyToastSettingEnabled()
{
    VERIFY_ARE_EQUAL(AppNotificationManager::Default().Setting(), AppNotificationSetting::Enabled);
}

void BaseTestSuite::VerifyToastPayload()
{
    winrt::hstring xmlPayload{ L"<toast>intrepidToast</toast>" };
    AppNotification toast{ xmlPayload };
    VERIFY_ARE_EQUAL(toast.Payload(), xmlPayload);
}

void BaseTestSuite::VerifyToastTag()
{
    AppNotification toast{ CreateToastNotification() };
    VERIFY_IS_TRUE(toast.Tag().empty());

    toast.Tag(L"Tag");
    VERIFY_ARE_EQUAL(toast.Tag(), L"Tag");
}

void BaseTestSuite::VerifyToastGroup()
{
    AppNotification toast{ CreateToastNotification() };
    VERIFY_IS_TRUE(toast.Group().empty());

    toast.Group(L"Group");
    VERIFY_ARE_EQUAL(toast.Group(), L"Group");
}

void BaseTestSuite::VerifyToastProgressDataFromToast()
{
    AppNotification toast{ CreateToastNotification() };

    AppNotificationProgressData progressData{ 1 };
    progressData.Status(L"Status");
    progressData.Title(L"Title");
    progressData.Value(0.14);
    progressData.ValueStringOverride(L"14%");
    toast.Progress(progressData);

    auto progressDataFromToast{ toast.Progress() };
    VERIFY_ARE_EQUAL(progressDataFromToast.Status(), L"Status");
    VERIFY_ARE_EQUAL(progressDataFromToast.Title(), L"Title");
    VERIFY_ARE_EQUAL(progressDataFromToast.Value(), 0.14);
    VERIFY_ARE_EQUAL(progressDataFromToast.ValueStringOverride(), L"14%");
    VERIFY_ARE_EQUAL(progressDataFromToast.SequenceNumber(), (uint32_t) 1);
}

void BaseTestSuite::VerifyToastExpirationTime()
{
    AppNotification toast{ CreateToastNotification() };
    VERIFY_ARE_EQUAL(toast.Expiration(), winrt::DateTime{});

    winrt::DateTime expirationTime{ winrt::clock::now() };
    expirationTime += winrt::TimeSpan{ std::chrono::seconds(10) };

    toast.Expiration(expirationTime);
    VERIFY_ARE_EQUAL(toast.Expiration(), expirationTime);
}

void BaseTestSuite::VerifyToastPriority()
{
    AppNotification toast{ CreateToastNotification() };
    VERIFY_ARE_EQUAL(toast.Priority(), AppNotificationPriority::Default);

    toast.Priority(AppNotificationPriority::High);
    VERIFY_ARE_EQUAL(toast.Priority(), AppNotificationPriority::High);
}

void BaseTestSuite::VerifyToastSuppressDisplay()
{
    AppNotification toast{ CreateToastNotification() };
    VERIFY_IS_FALSE(toast.SuppressDisplay());

    toast.SuppressDisplay(true);
    VERIFY_IS_TRUE(toast.SuppressDisplay());
}

void BaseTestSuite::VerifyToastExpiresOnReboot()
{
    AppNotification toast{ CreateToastNotification() };
    VERIFY_IS_FALSE(toast.ExpiresOnReboot());

    toast.ExpiresOnReboot(true);
    VERIFY_IS_TRUE(toast.ExpiresOnReboot());
}

void BaseTestSuite::VerifyToastProgressDataSequence0Fail()
{
    VERIFY_THROWS_HR(GetToastProgressData(L"PStatus", L"PTitle", 0.10, L"10%", 0), E_INVALIDARG);
}

void BaseTestSuite::VerifyShowToast()
{
    AppNotification toast{ CreateToastNotification() };
    AppNotificationManager::Default().Show(toast);
    VERIFY_ARE_NOT_EQUAL(toast.Id(), (uint32_t) 0);

    VerifyToastIsActive(toast.Id());
}

void BaseTestSuite::VerifyUpdateToastProgressDataUsingValidTagAndValidGroup()
{
    PostToastHelper(L"Tag", L"Group");
    AppNotificationProgressData progressData{ GetToastProgressData(L"Status", L"Title", 0.10, L"10%", 1) };

    auto progressResultOperation{ AppNotificationManager::Default().UpdateAsync(progressData, L"Tag", L"Group") };
    ProgressResultOperationHelper(progressResultOperation, winrt::AppNotificationProgressResult::Succeeded);
}

void BaseTestSuite::VerifyUpdateToastProgressDataUsingValidTagAndEmptyGroup()
{
    RegisterWithAppNotificationManager();
    PostToastHelper(L"Tag", L"");
    AppNotificationProgressData progressData{ GetToastProgressData(L"Status", L"Title", 0.14, L"14%", 1) };

    auto progressResultOperation{ AppNotificationManager::Default().UpdateAsync(progressData, L"Tag") };
    ProgressResultOperationHelper(progressResultOperation, winrt::AppNotificationProgressResult::Succeeded);
}

void BaseTestSuite::VerifyUpdateToastProgressDataUsingEmptyTagAndValidGroup()
{
    RegisterWithAppNotificationManager();
    AppNotificationProgressData progressData{ GetToastProgressData(L"Status", L"Title", 0.14, L"14%", 1) };

    VERIFY_THROWS_HR(AppNotificationManager::Default().UpdateAsync(progressData, L"", L"Group").get(), E_INVALIDARG);
}

void BaseTestSuite::VerifyUpdateToastProgressDataUsingEmptyTagAndEmptyGroup()
{
    RegisterWithAppNotificationManager();
    AppNotificationProgressData progressData{ GetToastProgressData(L"Status", L"Title", 0.14, L"14%", 1) };

    VERIFY_THROWS_HR(AppNotificationManager::Default().UpdateAsync(progressData, L"", L"").get(), E_INVALIDARG);
}

void BaseTestSuite::VerifyFailedUpdateNotificationDataWithNonExistentTagAndGroup()
{
    RegisterWithAppNotificationManager();
    PostToastHelper(L"Tag", L"Group");
    AppNotificationProgressData progressData{ GetToastProgressData(L"Status", L"Title", 0.14, L"14%", 1) };

    auto progressResultOperation{ AppNotificationManager::Default().UpdateAsync(progressData, L"NonExistentTag", L"NonExistentGroup") };
    ProgressResultOperationHelper(progressResultOperation, winrt::AppNotificationProgressResult::AppNotificationNotFound);
}

void BaseTestSuite::VerifyFailedUpdateNotificationDataWithoutPostToast()
{
    RegisterWithAppNotificationManager();
    AppNotificationProgressData progressData{ GetToastProgressData(L"Status", L"Title", 0.14, L"14%", 1) };

    auto progressResultOperation{ AppNotificationManager::Default().UpdateAsync(progressData, L"NonExistentTag", L"NonExistentGroup") };
    ProgressResultOperationHelper(progressResultOperation, winrt::AppNotificationProgressResult::AppNotificationNotFound);
}

void BaseTestSuite::VerifyGetAllAsyncWithZeroActiveToast()
{
    auto retrieveNotificationsAsync{ AppNotificationManager::Default().GetAllAsync() };
    auto scope_exit = wil::scope_exit(
    [&] {
        retrieveNotificationsAsync.Cancel();
    });

    VERIFY_ARE_EQUAL(retrieveNotificationsAsync.wait_for(std::chrono::seconds(300)), winrt::Windows::Foundation::AsyncStatus::Completed);
    scope_exit.release();

    VERIFY_ARE_EQUAL(retrieveNotificationsAsync.GetResults().Size(), (uint32_t) 0);
}

void BaseTestSuite::VerifyGetAllAsyncWithOneActiveToast()
{
    AppNotification toast{ CreateToastNotification(L"MyOwnLittleToast") };
    toast.Tag(L"aDifferentTag");
    toast.Group(L"aDifferentGroup");
    winrt::DateTime expirationTime{ winrt::clock::now() };
    expirationTime += winrt::TimeSpan{ std::chrono::seconds(10) };
    toast.Expiration(expirationTime);
    toast.ExpiresOnReboot(true);
    toast.Priority(winrt::AppNotificationPriority::High);
    toast.SuppressDisplay(true);

    AppNotificationProgressData progressData = GetToastProgressData(L"Status", L"Title", 0.14, L"14%", 1);
    toast.Progress(progressData);

    AppNotificationManager::Default().Show(toast);

    auto retrieveNotificationsAsync{ AppNotificationManager::Default().GetAllAsync() };
    auto scope_exit = wil::scope_exit(
        [&] {
            retrieveNotificationsAsync.Cancel();
        });

    VERIFY_ARE_EQUAL(retrieveNotificationsAsync.wait_for(std::chrono::seconds(300)), winrt::Windows::Foundation::AsyncStatus::Completed);
    scope_exit.release();

    auto notifications{ retrieveNotificationsAsync.GetResults() };
    VERIFY_ARE_EQUAL(notifications.Size(), (uint32_t) 1);

    auto actual = notifications.GetAt(0);
    return VerifyToastNotificationIsValid(toast, actual, ExpectedTransientProperties::DEFAULT);
}

void BaseTestSuite::VerifyGetAllAsyncWithMultipleActiveToasts()
{
    AppNotification toast1{ CreateToastNotification() };
    AppNotificationManager::Default().Show(toast1);

    AppNotification toast2{ CreateToastNotification() };
    AppNotificationManager::Default().Show(toast2);

    AppNotification toast3{ CreateToastNotification() };
    AppNotificationManager::Default().Show(toast3);

    auto retrieveNotificationsAsync{ AppNotificationManager::Default().GetAllAsync() };
    auto scope_exit = wil::scope_exit(
        [&] {
            retrieveNotificationsAsync.Cancel();
        });

    VERIFY_ARE_EQUAL(retrieveNotificationsAsync.wait_for(std::chrono::seconds(300)), winrt::Windows::Foundation::AsyncStatus::Completed);
    scope_exit.release();

    auto notifications{ retrieveNotificationsAsync.GetResults() };
    VERIFY_ARE_EQUAL(notifications.Size(), (uint32_t) 3);
    VERIFY_ARE_EQUAL(L"<toast>intrepidToast</toast>", notifications.GetAt(0).Payload());
}

void BaseTestSuite::VerifyGetAllAsyncIgnoresUpdatesToProgressData()
{
    AppNotification toast{ CreateToastNotification() };
    toast.Tag(L"Tag");
    toast.Group(L"Group");
    AppNotificationProgressData initialProgressData{ GetToastProgressData(L"Initial Status", L"Initial Title", 0.05, L"5%", 1) };
    toast.Progress(initialProgressData);

    AppNotificationManager::Default().Show(toast);

    winrt::AppNotificationProgressData updatedProgressData{ GetToastProgressData(L"Updated Status", L"Updated Title", 0.14, L"14%", 1) };
    auto progressResultOperation{ AppNotificationManager::Default().UpdateAsync(updatedProgressData, L"Tag", L"Group") };
    ProgressResultOperationHelper(progressResultOperation, winrt::AppNotificationProgressResult::Succeeded);

    auto retrieveNotificationsAsync{ AppNotificationManager::Default().GetAllAsync() };
    auto scope_exit = wil::scope_exit(
    [&] {
        retrieveNotificationsAsync.Cancel();
    });

    VERIFY_ARE_EQUAL(retrieveNotificationsAsync.wait_for(std::chrono::seconds(300)), winrt::Windows::Foundation::AsyncStatus::Completed);
    scope_exit.release();

    auto notifications{ retrieveNotificationsAsync.GetResults() };
    VERIFY_ARE_EQUAL(notifications.Size(), (uint32_t) 1);
    VerifyToastNotificationIsValid(toast, notifications.GetAt(0), ExpectedTransientProperties::EQUAL);
}

void BaseTestSuite::VerifyRemoveWithIdentifierAsyncUsingZeroedToastIdentifier()
{
    auto removeNotificationAsync{ AppNotificationManager::Default().RemoveByIdAsync(0) };

    VERIFY_ARE_EQUAL(removeNotificationAsync.wait_for(std::chrono::seconds(300)), winrt::Windows::Foundation::AsyncStatus::Error);
    VERIFY_THROWS_HR(removeNotificationAsync.GetResults(), E_INVALIDARG);
    removeNotificationAsync.Cancel();
}

void BaseTestSuite::VerifyRemoveWithIdentifierAsyncUsingNonActiveToastIdentifierDoesNotThrow()
{
    AppNotification toast{ CreateToastNotification() };
    AppNotificationManager::Default().Show(toast);
    auto id{ toast.Id() };

    AppNotificationManager::Default().RemoveAllAsync().get();

    auto removeNotificationAsync{ AppNotificationManager::Default().RemoveByIdAsync(id) };
    auto scope_exit = wil::scope_exit(
    [&] {
        removeNotificationAsync.Cancel();
    });

    VERIFY_ARE_EQUAL(removeNotificationAsync.wait_for(std::chrono::seconds(300)), winrt::Windows::Foundation::AsyncStatus::Completed);
    scope_exit.release();

    removeNotificationAsync.GetResults();
}

void BaseTestSuite::VerifyRemoveWithIdentifierAsyncUsingActiveToastIdentifier()
{
    winrt::AppNotification toast1{ CreateToastNotification(L"Toast1") };
    AppNotificationManager::Default().Show(toast1);

    winrt::AppNotification toast2{ CreateToastNotification(L"Toast2") };
    AppNotificationManager::Default().Show(toast2);

    winrt::AppNotification toast3{ CreateToastNotification(L"Toast3") };
    AppNotificationManager::Default().Show(toast3);

    VerifyToastIsActive(toast1.Id());
    VerifyToastIsActive(toast2.Id());
    VerifyToastIsActive(toast3.Id());

    AppNotificationManager::Default().RemoveByIdAsync(toast2.Id()).get();

    VerifyToastIsActive(toast1.Id());
    VerifyToastIsActive(toast3.Id());
    VerifyToastIsInactive(toast2.Id());
}

void BaseTestSuite::VerifyRemoveWithTagAsyncUsingEmptyTagThrows()
{
    VERIFY_THROWS_HR(AppNotificationManager::Default().RemoveByTagAsync(L"").get(), E_INVALIDARG);
}

void BaseTestSuite::VerifyRemoveWithTagAsyncUsingNonExistentTagDoesNotThrow()
{
    auto removeNotificationAsync{ AppNotificationManager::Default().RemoveByTagAsync(L"NonExistentTag") };
    auto scope_exit = wil::scope_exit(
        [&] {
            removeNotificationAsync.Cancel();
        });

    VERIFY_ARE_EQUAL(removeNotificationAsync.wait_for(std::chrono::seconds(300)), winrt::Windows::Foundation::AsyncStatus::Completed);
    scope_exit.release();
}

void BaseTestSuite::VerifyRemoveWithTagAsync()
{
    winrt::AppNotification toast{ CreateToastNotification() };
    toast.Tag(L"Unique tag");
    AppNotificationManager::Default().Show(toast);

    VerifyToastIsActive(toast.Id());

    auto removeNotificationAsync{ AppNotificationManager::Default().RemoveByTagAsync(L"Unique tag") };
    auto scope_exit = wil::scope_exit(
    [&] {
        removeNotificationAsync.Cancel();
    });

    VERIFY_ARE_EQUAL(removeNotificationAsync.wait_for(std::chrono::seconds(300)), winrt::Windows::Foundation::AsyncStatus::Completed);
    scope_exit.release();

    VerifyToastIsInactive(toast.Id());
}

void BaseTestSuite::VerifyRemoveWithTagGroupAsyncUsingEmptyTagThrows()
{
    VERIFY_THROWS_HR(AppNotificationManager::Default().RemoveByTagAndGroupAsync(L"", L"NonexistentGroup").get(), E_INVALIDARG);
}

void BaseTestSuite::VerifyRemoveWithTagGroupAsyncUsingEmptyGroupThrows()
{
    VERIFY_THROWS_HR(AppNotificationManager::Default().RemoveByTagAndGroupAsync(L"NonexistentTag", L"").get(), E_INVALIDARG);
}

void BaseTestSuite::VerifyRemoveWithTagGroupAsync()
{
    AppNotification toast1{ CreateToastNotification() };
    toast1.Tag(L"tag1");
    toast1.Group(L"Shared group");
    AppNotificationManager::Default().Show(toast1);

    winrt::AppNotification toast2{ CreateToastNotification() };
    toast2.Tag(L"tag2");
    toast2.Group(L"Shared group");
    AppNotificationManager::Default().Show(toast2);

    winrt::AppNotification toast3{ CreateToastNotification() };
    toast3.Tag(L"tag3");
    toast3.Group(L"Shared group");
    AppNotificationManager::Default().Show(toast3);

    VerifyToastIsActive(toast1.Id());
    VerifyToastIsActive(toast2.Id());
    VerifyToastIsActive(toast3.Id());

    auto removeNotificationAsync{ AppNotificationManager::Default().RemoveByTagAndGroupAsync(L"tag2", L"Shared group") };
    auto scope_exit = wil::scope_exit(
        [&] {
            removeNotificationAsync.Cancel();
        });

    VERIFY_ARE_EQUAL(removeNotificationAsync.wait_for(std::chrono::seconds(300)), winrt::Windows::Foundation::AsyncStatus::Completed);
    scope_exit.release();

    VerifyToastIsActive(toast1.Id());
    VerifyToastIsInactive(toast2.Id());
    VerifyToastIsActive(toast3.Id());
}

void BaseTestSuite::VerifyRemoveGroupAsyncUsingEmptyGroupThrows()
{
    VERIFY_THROWS_HR(AppNotificationManager::Default().RemoveByGroupAsync(L"").get(), E_INVALIDARG);
}

void BaseTestSuite::VerifyRemoveGroupAsyncUsingNonExistentGroupDoesNotThrow()
{
    auto removeNotificationAsync{ AppNotificationManager::Default().RemoveByGroupAsync(L"group") };
    auto scope_exit = wil::scope_exit(
        [&] {
            removeNotificationAsync.Cancel();
        });

    VERIFY_ARE_EQUAL(removeNotificationAsync.wait_for(std::chrono::seconds(300)), winrt::Windows::Foundation::AsyncStatus::Completed);
    scope_exit.release();
}

void BaseTestSuite::VerifyRemoveGroupAsync()
{
    winrt::AppNotification toast1{ CreateToastNotification() };
    toast1.Tag(L"tag1");
    toast1.Group(L"Shared group");
    AppNotificationManager::Default().Show(toast1);

    winrt::AppNotification toast2{ CreateToastNotification() };
    toast2.Tag(L"tag2");
    toast2.Group(L"Shared group");
    AppNotificationManager::Default().Show(toast2);

    winrt::AppNotification toast3{ CreateToastNotification() };
    toast3.Tag(L"tag3");
    toast3.Group(L"Shared group");
    AppNotificationManager::Default().Show(toast3);

    VerifyToastIsActive(toast1.Id());
    VerifyToastIsActive(toast2.Id());
    VerifyToastIsActive(toast3.Id());

    auto removeNotificationAsync{ AppNotificationManager::Default().RemoveByGroupAsync(L"Shared group") };
    auto scope_exit = wil::scope_exit(
        [&] {
            removeNotificationAsync.Cancel();
        });

    VERIFY_ARE_EQUAL(removeNotificationAsync.wait_for(std::chrono::seconds(300)), winrt::Windows::Foundation::AsyncStatus::Completed);
    scope_exit.release();

    VerifyToastIsInactive(toast1.Id());
    VerifyToastIsInactive(toast2.Id());
    VerifyToastIsInactive(toast3.Id());
}

void BaseTestSuite::VerifyRemoveAllAsyncWithNoActiveToastDoesNotThrow()
{
    auto removeNotificationAsync{ AppNotificationManager::Default().RemoveAllAsync() };
    auto scope_exit = wil::scope_exit(
        [&] {
            removeNotificationAsync.Cancel();
        });

    VERIFY_ARE_EQUAL(removeNotificationAsync.wait_for(std::chrono::seconds(300)), winrt::Windows::Foundation::AsyncStatus::Completed);
    scope_exit.release();
}

void BaseTestSuite::VerifyRemoveAllAsync()
{
    AppNotification toast1{ CreateToastNotification() };
    AppNotificationManager::Default().Show(toast1);

    AppNotification toast2{ CreateToastNotification() };
    AppNotificationManager::Default().Show(toast2);

    AppNotification toast3{ CreateToastNotification() };
    AppNotificationManager::Default().Show(toast3);

    auto getAllAsync{ AppNotificationManager::Default().GetAllAsync() };
    auto scopeExitGetAll = wil::scope_exit(
        [&] {
            getAllAsync.Cancel();
        });

    VERIFY_ARE_EQUAL(getAllAsync.wait_for(std::chrono::seconds(300)), winrt::Windows::Foundation::AsyncStatus::Completed);
    scopeExitGetAll.release();

    VERIFY_ARE_EQUAL(getAllAsync.GetResults().Size(), (uint32_t) 3);

    auto removeAllAsync{ AppNotificationManager::Default().RemoveAllAsync() };
    auto scopeExitRemoveAll = wil::scope_exit(
        [&] {
            removeAllAsync.Cancel();
        });

    VERIFY_ARE_EQUAL(removeAllAsync.wait_for(std::chrono::seconds(300)), winrt::Windows::Foundation::AsyncStatus::Completed);
    scopeExitRemoveAll.release();

    auto getAllAsync2{ AppNotificationManager::Default().GetAllAsync() };
    auto scopeExitGetAll2 = wil::scope_exit(
        [&] {
            getAllAsync2.Cancel();
        });

    VERIFY_ARE_EQUAL(getAllAsync2.wait_for(std::chrono::seconds(300)), winrt::Windows::Foundation::AsyncStatus::Completed);
    scopeExitGetAll2.release();

    VERIFY_ARE_EQUAL(getAllAsync2.GetResults().Size(), (uint32_t) 0);
}

void BaseTestSuite::VerifyIconPathExists()
{
    //try
    //{
    //    // Register is already called in main with an explicit appusermodelId
    //    wil::unique_cotaskmem_string localAppDataPath;
    //    THROW_IF_FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0 /* flags */, nullptr /* access token handle */, &localAppDataPath));

    //    // Evaluated path: C:\Users\<currentUser>\AppData\Local\Microsoft\WindowsAppSDK\<AUMID>.png
    //    std::path iconFilePath{ std::wstring(localAppDataPath.get()) + c_localWindowsAppSDKFolder + c_appUserModelId + c_pngExtension };
    //    winrt::check_bool(std::exists(iconFilePath));

    //    winrt::AppNotificationManager::Default().UnregisterAll();

    //    // After unregister this file should not exist
    //    winrt::check_bool(!std::exists(iconFilePath));
    //}
    //catch (...)
    //{
    //    return false;
    //}

    //return true;
}

void BaseTestSuite::VerifyExplicitAppId()
{
    if (!Test::AppModel::IsPackagedProcess())
    {
        // Not mandatory, but it's highly recommended to specify AppUserModelId
        THROW_IF_FAILED(SetCurrentProcessExplicitAppUserModelID(c_appUserModelId.c_str()));
    }
   /* winrt::AppNotificationManager::Default().Unregister();
    try
    {
        winrt::AppNotificationManager::Default().Register();
        winrt::AppNotificationManager::Default().Unregister();
    }
    catch (...)
    {
        return false;
    }
    return true;*/
}
